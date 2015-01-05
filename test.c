#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "gfx.h"
#include "test.h"
#include "graphicsProcessing.h"
#include "networking.h"

#define LEFT 1
#define RIGHT 2
#define UP 4
#define DOWN 8
#define TOP 16
#define BOTTOM 32
#define STRANGE 64
#define CHARMED 128

#define LOOKSIN sin(LOOKSPD)
#define LOOKCOS cos(LOOKSPD)
#define MOVESPD 5000
#define GROUNDFRIC 500
#define LOOKSPD .03

#define NUMCYCLES 3
static double cycleYaw[NUMCYCLES] = {0, 1.2, 0.6};
static double cyclePitch[NUMCYCLES] = {0, 0, -.3};
static int viewCycle = 0;
static char usingCycles = 1;

box* boxen = NULL;
int boxCapacity = 0;
int numBoxen = 0;

player me = {.pos = {0, 0, 0, 0}, .vel={0}, .view={{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}}, .yaw = 0, .fitch = 0, .pitch = 0, .canJump = 0};

static char lookingAround = 0;

box* addBox(){
	numBoxen++;
	if (numBoxen>boxCapacity){
		boxCapacity += 5;
		boxen = realloc(boxen, boxCapacity*sizeof(box));
	}
	box* ret = boxen+numBoxen-1;
	ret->lines = malloc(5*sizeof(line));
	ret->numLines = 0;
	ret->lineCapacity = 5;
	return ret;
}

/*void playerRollChange(player* who, int dir){
	double one[3];
	memcpy(one, who->view[1], 3*sizeof(double));
	double two[3];
	memcpy(two, who->view[2], 3*sizeof(double));
	double mysin=dir*LOOKSIN;
	int i = 0;
	for(; i < 3; i++){
		who->view[1][i] = LOOKCOS*one[i] + mysin*two[i];
		who->view[2][i] = LOOKCOS*two[i] - mysin*one[i];
	}
}
void playerPitchChange(player* who, int dir){
	double zero[3];
	memcpy(zero, who->fauxView, 3*sizeof(double));
	double two[3];
	memcpy(two, who->view[2], 3*sizeof(double));
	double mysin=dir*LOOKSIN;
	int i = 0;
	for(; i < 3; i++){
		who->fauxView[i] = LOOKCOS*zero[i] + mysin*two[i];
		who->view[2][i] = LOOKCOS*two[i] - mysin*zero[i];
	}
}
void playerYawChange(player* who, int dir){
	double zero[3];
	memcpy(zero, who->fauxView, 3*sizeof(double));
	double one[3];
	memcpy(one, who->view[1], 3*sizeof(double));
	double mysin=dir*LOOKSIN;
	int i = 0;
	for(; i < 3; i++){
		who->fauxView[i] = LOOKCOS*zero[i] + mysin*one[i];
		who->view[1][i] = LOOKCOS*one[i] - mysin*zero[i];
	}
}*/

void normalize3d(double* w){
	double dist = 1.0/sqrt(w[0]*w[0] + w[1]*w[1] + w[2]*w[2]);
	if (dist == 0) return;
	int i = 0;
	for(; i < 3; i++){
		w[i] *= dist;
	}
}

void setPlayerViewVectors(player *who){
	who->view[1][1] = who->view[0][0] = cos(who->yaw);
	who->view[1][0] = -(who->view[0][1] = sin(who->yaw));

	double Cos = cos(who->fitch*FITCHTOANGLE);
	double Sin = sin(who->fitch*FITCHTOANGLE);

	who->view[3][0] = -Sin*who->view[0][0];
	who->view[3][1] = -Sin*who->view[0][1];
	who->view[3][3] = Cos;

	who->moveVec[0] = (who->view[0][0] *= Cos);
	who->moveVec[1] = (who->view[0][1] *= Cos);
	who->moveVec[2] = (who->view[0][3] = Sin);

	Sin = sin(who->pitch);
	Cos = cos(who->pitch);

	who->view[2][0] = -Sin*who->view[0][0];
	who->view[2][1] = -Sin*who->view[0][1];
	who->view[2][3] = -Sin*who->view[0][3];
	who->view[2][2] = Cos;

	who->view[0][0] *= Cos;
	who->view[0][1] *= Cos;
	who->view[0][3] *= Cos;
	who->view[0][2] = Sin;
}

SDL_Window *window;
SDL_Texture *texture;
SDL_Renderer *renderer;

void doPlayerPhysics(player* who){
	who->canJump = 0;
	double dx = who->desiredVel[0] - who->vel[0];
	double dy = who->desiredVel[1] - who->vel[1];
	double dz = who->desiredVel[2] - who->vel[3];
	double mag = sqrt(dx*dx + dy*dy + dz*dz);
	if (mag > GROUNDFRIC){
		mag = GROUNDFRIC/mag;
		dx *= mag;
		dy *= mag;
		dz *= mag;
	}
	who->vel[0] += dx;
	who->vel[1] += dy;
	who->vel[3] += dz;
	who->vel[2] -= 1000;

	int minTime, timeLeft = 1000;
	int i, j;
	int intStart, intEnd;
	int tmp1;
	unsigned char dims[4];
	int numDims; // Keeps track of how many dimensions we are simultaneously running into walls in
	unsigned char dims2[4];
	int numDims2;
	int sizeSum;
	while(timeLeft){
		minTime = timeLeft;
		numDims = 0;
		for(i=0; i<numBoxen; i++){
			numDims2 = 0;
			intStart = 0;
			intEnd = minTime;
			for(j=0; j<4; j++){
				if (who->vel[j]<0){
					sizeSum = -boxen[i].size[j]-playerSize;
				}else{
					sizeSum = boxen[i].size[j]+playerSize;
					if (who->vel[j] == 0){
						if (abs(boxen[i].pos[j]-who->pos[j]) < sizeSum) continue;
						numDims2 = 0;
						break;
					}
				}
				tmp1 = 1000*(boxen[i].pos[j]-who->pos[j]-sizeSum)/who->vel[j];
				if (tmp1>intStart){
					if (tmp1>intEnd){
						numDims2 = 0;
						break;
					}
					intStart = tmp1;
					dims2[0] = j;
					numDims2 = 1;
				}else if (tmp1 == intStart){
					dims2[numDims2++] = j;
				}
				tmp1 = 1000*(boxen[i].pos[j]-who->pos[j]+sizeSum)/who->vel[j];
				if (tmp1<intEnd){
					if (tmp1<intStart){
						numDims2 = 0;
						break;
					}
					intEnd = tmp1;
				}
			}
			if (numDims2 == 0 || (intStart==minTime && numDims2>=numDims)) continue; // Prioritize running into fewer dimensions at once, due to the tiled floor issue
			minTime = intStart;
			numDims = numDims2;
			memcpy(dims, dims2, numDims);
		}
		timeLeft -= minTime;
		for(j=0; j<4; j++){
			who->pos[j] += who->vel[j]*minTime/1000;
		}
		for(j=0; j<numDims; j++){
			if (dims[j] == 2 && who->vel[2] < 0) //This is a little bit kludgey - If we ever have moving platforms, our velocity might be positive even though we're colliding with something below us.
				who->canJump = 1;
			who->vel[dims[j]] = 0;
		}
	}
}

static void interpretKeys(player *me, uint16_t moveKeys) {
	me->desiredVel[0] = me->desiredVel[1] = me->desiredVel[2] = 0;
	if (moveKeys&UP){
		me->desiredVel[0] += MOVESPD*me->moveVec[0];
		me->desiredVel[1] += MOVESPD*me->moveVec[1];
		me->desiredVel[2] += MOVESPD*me->moveVec[2];
	}
	if (moveKeys&DOWN){
		me->desiredVel[0] -= MOVESPD*me->moveVec[0];
		me->desiredVel[1] -= MOVESPD*me->moveVec[1];
		me->desiredVel[2] -= MOVESPD*me->moveVec[2];
	}
	if (moveKeys&LEFT){
		me->desiredVel[0] -= MOVESPD*me->view[1][0];
		me->desiredVel[1] -= MOVESPD*me->view[1][1];
	}
	if (moveKeys&RIGHT){
		me->desiredVel[0] += MOVESPD*me->view[1][0];
		me->desiredVel[1] += MOVESPD*me->view[1][1];
	}
	if (moveKeys&TOP){
		me->desiredVel[0] -= MOVESPD*me->view[3][0];
		me->desiredVel[1] -= MOVESPD*me->view[3][1];
		me->desiredVel[2] -= MOVESPD*me->view[3][3];
	}
	if (moveKeys&BOTTOM){
		me->desiredVel[0] += MOVESPD*me->view[3][0];
		me->desiredVel[1] += MOVESPD*me->view[3][1];
		me->desiredVel[2] += MOVESPD*me->view[3][3];
	}
	if (moveKeys&CHARMED && me->canJump) me->vel[2] += 10000;
	if (moveKeys&STRANGE) me->vel[2] -= MOVESPD;
}

int main(int argc, char** argv){
	if (argc > 2) {
		initNetwork(argc, argv);
	}

	initGraphics();

	Uint16 moveKeys = 0;

	struct timespec t;
	t.tv_sec = 0;
	struct timespec lastTime = {.tv_sec = 0, .tv_nsec = 0};
	struct timespec otherTime = {.tv_sec = 0, .tv_nsec = 0};
	
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	width2 = height2 = 400;
	window = SDL_CreateWindow("4D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width2*2, height2*2, SDL_WINDOW_OPENGL);
	if (window == NULL){
		fputs("No SDL2 window.\n", stderr);
		fputs(SDL_GetError(), stderr);
		SDL_Quit();
		return 1;
	}
	SDL_GLContext context = SDL_GL_CreateContext(window);
	initGfx();
	glClearColor(0, 0, 0, 1);
	int running=1;
	/*int map[25] = {
		9,   9,   9,   9,  17,
		9,   8,   1,   5,   2,
		9,   9,   9,   1,   1,
		9,   1,   9,   1,   1,
		9,   9,   9,   1,   1};*/
	int map[25] = {
		1,   1,   1,   1,   1,
		1,   1,   9,   1,   1,
		1,   1,   1,   1,   1,
		1,   1,  15,   7,   3,
		1,   1,   1,   1,   1};
	box *ptr;
	int i = 0, j, k;
	for (; i < 5; i++) {
		for (j = 0; j < 5; j++) {
			for (k = 0; k < 5; k++) {
				if (map[i * 5 + j] & (1 << k)) {
					ptr = addBox();
					ptr->pos[0]= 80000 - 40000*i;
					ptr->pos[1]=-80000 + 40000*j;
					ptr->pos[3]=-80000 + 40000*k;
					ptr->pos[2]=0;
					ptr->size[0] = ptr->size[1] = ptr->size[2] = ptr->size[3] = 20000;
					ptr->color = 0xFF0000FF;
				}
				ptr = addBox();
				ptr->pos[0]= 80000 - 40000*i;
				ptr->pos[1]=-80000 + 40000*j;
				ptr->pos[3]=-80000 + 40000*k;
				ptr->pos[2]=-40000;
				ptr->size[0] = ptr->size[1] = ptr->size[2] = ptr->size[3] = 20000;
				ptr->color = 0xFF0000FF;
			}
		}
	}
	
	for(i = 0; i < 3; i++){
		for(j=0; j<4; j++){
			viewSpaceBoundingLines[4*i+j].pos[i] = -1;
			viewSpaceBoundingLines[4*i+j].pos[(i+1)%3] = primaryDim[j]*2-1;
			viewSpaceBoundingLines[4*i+j].pos[(i+2)%3] = secondaryDim[j]*2-1;
			viewSpaceBoundingLines[4*i+j].color = 0x0000FFFF;
			viewSpaceBoundingLines[4*i+j].dir[i] = 1;
			viewSpaceBoundingLines[4*i+j].dir[(i+1)%3] = 0;
			viewSpaceBoundingLines[4*i+j].dir[(i+2)%3] = 0;
			viewSpaceBoundingLines[4*i+j].len = 2;
			viewSpaceBoundingLines[4*i+j].major = 0;
		}
	}
	SDL_SetRelativeMouseMode(1);
	while(running){
		clock_gettime(CLOCK_MONOTONIC, &otherTime);
		long int sleep = 50000000 - (otherTime.tv_nsec-lastTime.tv_nsec+1000000000*(otherTime.tv_sec-lastTime.tv_sec));
		if (sleep > 0){
			t.tv_nsec = sleep;
			nanosleep(&t, NULL);
		}
		clock_gettime(CLOCK_MONOTONIC, &lastTime);
		SDL_Event evnt;
		while(SDL_PollEvent(&evnt)){
			if (evnt.type == SDL_QUIT) running = 0;
			else if (evnt.type == SDL_MOUSEBUTTONDOWN) {
				SDL_SetRelativeMouseMode(1);
			} else if (evnt.type == SDL_MOUSEWHEEL){
					me.fitch += evnt.wheel.y;
					if (me.fitch > MAXFITCH) me.fitch = MAXFITCH;	
					else if (me.fitch < -MAXFITCH) me.fitch = -MAXFITCH;
			}
			else if (evnt.type == SDL_MOUSEMOTION){
				if (!SDL_GetRelativeMouseMode()) continue;
				if (lookingAround) {
					yaw3d -= evnt.motion.xrel*.004;
					pitch3d -= evnt.motion.yrel*.004;
					if (yaw3d > M_PI) yaw3d -= 2*M_PI;
					else if (yaw3d < -M_PI) yaw3d += 2*M_PI;
					if (pitch3d > M_PI_2) pitch3d = M_PI_2;
					else if (pitch3d < -M_PI_2) pitch3d = -M_PI_2;
				} else {
					me.yaw += evnt.motion.xrel*.004;
					me.pitch -= evnt.motion.yrel*.004;
					if (me.yaw > M_PI) me.yaw -= 2*M_PI;
					else if (me.yaw < -M_PI) me.yaw += 2*M_PI;
					if (me.pitch > M_PI_2) me.pitch = M_PI_2;
					else if (me.pitch < -M_PI_2) me.pitch = -M_PI_2;
				}
			}
			else if (evnt.type == SDL_KEYDOWN || evnt.type == SDL_KEYUP){
				int thing = 0;
				switch(evnt.key.keysym.sym){
					case SDLK_s:
						thing = DOWN;
						break;
					case SDLK_w:
						thing = UP;
						break;
					case SDLK_a:
						thing = LEFT;
						break;
					case SDLK_d:
						thing = RIGHT;
						break;
					case SDLK_SPACE:
						thing = CHARMED;
						break;
					case SDLK_LSHIFT:
						thing = STRANGE;
						break;
					case SDLK_f:
						thing = TOP;
						break;
					case SDLK_r:
						thing = BOTTOM;
						break;
					case SDLK_q:
						lookingAround = (evnt.type == SDL_KEYDOWN);
						usingCycles = 0;
						break;
					case SDLK_v:
						displayFlag = (evnt.type == SDL_KEYDOWN);
						break;
					case SDLK_TAB:
						if (evnt.type == SDL_KEYUP) break;
						if (usingCycles)
							viewCycle = (viewCycle+1) % NUMCYCLES;
						else 
							usingCycles = 1;
						break;
					case SDLK_RETURN:
						me.pos[0] =
						me.pos[1] =
						me.pos[2] =
						me.pos[3] = 0;
						break;
					case SDLK_ESCAPE:
						SDL_SetRelativeMouseMode(0);
						break;
					default:
						break;
				}
				if (evnt.type == SDL_KEYDOWN)
					moveKeys = moveKeys|thing;
				else
					moveKeys = moveKeys&(~thing);
			}
		}
		if (usingCycles) {
			yaw3d += 0.1*(cycleYaw[viewCycle] - yaw3d);
			pitch3d += 0.1*(cyclePitch[viewCycle] - pitch3d);
		}
		set3dViewVectors();		
		setPlayerViewVectors(&me);
		interpretKeys(&me, moveKeys);
		doPlayerPhysics(&me);
		if (networkActive) netTick();
		glClear(GL_COLOR_BUFFER_BIT);
		int i = 0;
		for(; i < numBoxen; i++) render(&me, boxen+i);
		occlude(&me);
		renderLines();
		SDL_GL_SwapWindow(window);
	}
	SDL_GL_DeleteContext(context);
	SDL_Quit();
	if (networkActive) {
		stopNetwork();
	}
	return 0;
}
