#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "test.h"
#include "graphicsProcessing.h"
#include "gfx.h"

static renderData getRenderData(player *who, double pos[4]);

uint32_t* screen;

int primaryDim[4] = {0, 1, 1, 0};
int secondaryDim[4] = {0, 0, 1, 1};

double viewerPos[3] = {0, 0, 3};
char headOnView = 0; // When we're looking into our 3-space viewing cube from straight ahead (a head-on view), the calculation become simpler
double view3d[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}; // These are the vectors that define a head-on view
double yaw3d = 0, pitch3d = 0;

#define indicatorStart (-.8)
#define indicatorStep 0.4

#ifdef IND
static int allIndicators[maxIndicators];
static double realIndicators[maxIndicators][3];
#endif

void set3dViewVectors() {
	double sinYaw = sin(yaw3d);
	double cosYaw = cos(yaw3d);
	double sinPitch = sin(pitch3d);
	double cosPitch = cos(pitch3d);

//view3d[0], view3d[1], and view3d[2] are the vectors which point to our right, up, and away from us, respectively.
//	view3d[0][1] = 0; // But this isn't going to change anyway
	view3d[0][0] = cosYaw;
	view3d[0][2] = sinYaw;

	viewerPos[0] = 4 * (view3d[2][0] = -sinYaw * cosPitch);
	viewerPos[2] = 4 * (view3d[2][2] = cosYaw * cosPitch);
	viewerPos[1] = 4 * (view3d[2][1] = sinPitch);

	view3d[1][1] = cosPitch;
	view3d[1][0] = sinYaw * sinPitch; // These two components are just -sinPitch * the corresponding component of view3d[2]
	view3d[1][2] = cosYaw * -sinPitch;
}

void initGraphics(){
#ifdef IND
	int i = 0, j, k, t;
	for(; i < indicatorDim; i++)
		for(j=0; j < indicatorDim; j++)
			for(k=0; k < indicatorDim; k++){
				t = i*indicatorDim*indicatorDim + j*indicatorDim + k;
				allIndicators[t]=t;
				realIndicators[t][0]=indicatorStart+indicatorStep*i;
				realIndicators[t][1]=indicatorStart+indicatorStep*j;
				realIndicators[t][2]=indicatorStart+indicatorStep*k;
			}
#endif
}

//Mayhaps I should use SIMD on this? (Single instruction multiple data) (But I'm too lazy)
#define dot3d(a, b) (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])
void renderLineSingular(line* cur){//This renders 3-space lines in 2-space.
	setColorFromHex(cur->color);
	if (headOnView) {
		double factor = 2.0/(viewerPos[2]+cur->pos[2]);
		double factor2 = 2.0/(viewerPos[2]+cur->pos[2] + cur->len*cur->dir[2]);
		double posx = cur->pos[0] + viewerPos[0];
		double posy = cur->pos[1] + viewerPos[1];
		drawLine(posx*factor, posy*factor, (posx+cur->dir[0]*cur->len)*factor2, (posy+cur->dir[1]*cur->len)*factor2);
	} else {
		double coords[3], coords2[3];
		int i = 0;
		for (; i < 3; i++) {
			coords[i] = cur->pos[i] + viewerPos[i];
			coords2[i] = coords[i] + cur->dir[i] * cur->len;
		}
		double factor = 2.0/dot3d(coords, view3d[2]);
		double factor2 = 2.0/dot3d(coords2, view3d[2]);
		drawLine(dot3d(coords, view3d[0])*factor, dot3d(coords, view3d[1])*factor, dot3d(coords2, view3d[0])*factor2, dot3d(coords2, view3d[1])*factor2);
	}
}

void renderLineGuides(line* cur){
	if(cur->major == 0) return;
	setColorFromHex(0x404040FF);
	if(cur->major & 1){//Perhaps change this later so we only draw guide lines from unoccluded corners.
		double factor = 2.0/(3+cur->pos[2]);
		//lineColor(screen, (int)(cur->pos[0]+250), (int)(cur->pos[1]+250), (int)(cur->pos[0]*500.0/1000 + 250), (int)(cur->pos[1]*500.0/1000 + 250), 0x404040FF);
		drawLine(-factor, cur->pos[1]*factor, factor, cur->pos[1]*factor);
		drawLine(cur->pos[0]*factor, -factor, cur->pos[0]*factor, factor);
	}
	if(cur->major & 2){//Perhaps change this later so we only draw guide lines from unoccluded corners.
		double x = cur->pos[0] + cur->len*cur->dir[0];
		double y = cur->pos[1] + cur->len*cur->dir[1];
		double factor = 2.0/(3+cur->pos[2]+cur->len*cur->dir[2]);
		drawLine(-factor, y*factor, factor, y*factor);
		drawLine(x*factor, -factor, x*factor, factor);
	}
}

#ifdef IND
static void renderDemIndicators(box* who){
	int i = who->numIndicators-1;
	setColorFromHex(who->color);
	for(; i >= 0; i--){
		double *ind = realIndicators[who->indicators[i]];
		double factor = 2.0/(viewerPos[2]+ind[2]);
		double posx = (ind[0] + viewerPos[0])*factor;
		double posy = (ind[1] + viewerPos[1])*factor;
		double delta = .03*factor;
		drawLine(posx-delta, posy, posx+delta, posy);
		drawLine(posx, posy-delta, posx, posy+delta);
	}
}
#endif

line viewSpaceBoundingLines[12];

void renderLines(){
	int i, j;
	for(i=0; i<12; i++) renderLineSingular(viewSpaceBoundingLines+i);
	for(i=0; i < numBoxen; i++){
		//for(j=0; j<boxen[i].numLines; j++) renderLineGuides(boxen[i].lines+j);
#ifdef IND
		renderDemIndicators(boxen+i);
#endif
	}
	for(i=0; i < numBoxen; i++){
		for(j=0; j<boxen[i].numLines; j++) renderLineSingular(boxen[i].lines+j);
		boxen[i].numLines = 0;
	}
}

char displayFlag;

line* allocLine(box* who){
	if(++who->numLines == who->lineCapacity)
		who->lines = realloc(who->lines, sizeof(line)*(who->lineCapacity+=5));
	return who->lines+who->numLines-1;
}

void addLine(box *who, double x, double y, double z, double dx, double dy, double dz, int color, char major){
	if(dx == 0 && dy == 0 && dz == 0) return;//Save /0 errors and time. (Change && to ||? fix more /0 errors?)
	line *lL = who->lines+who->numLines;//Don't increment numLines until after we're sure it worked
	lL->major = major;
	lL->len = sqrt(dx*dx + dy*dy + dz*dz);
	lL->dir[0] = dx/lL->len;
	lL->dir[1] = dy/lL->len;
	lL->dir[2] = dz/lL->len;
	lL->pos[0] = x;
	lL->pos[1] = y;
	lL->pos[2] = z;
	lL->color = color;
	double times[3];
	int i = 0;
	for(; i < 3; i++){
		if(lL->dir[i] == 0){
			if(fabs(lL->pos[i])>1) return;
			times[i] = -HUGE_VAL;
		}else{
			times[i] = (lL->dir[i]<0?1-lL->pos[i]:-1-lL->pos[i])/lL->dir[i];
		}
	}
	unsigned char max = times[1] > times[0];
	max = (times[max] > times[2]) ? max : 2;
	//if(color == 8323327) printf("%d ", max);
		
	if(times[max] > 0){
		if(times[max]>=lL->len) return;//doesn't reach the viewing area
		for(i = 0; i < 3; i++) lL->pos[i] += lL->dir[i]*times[max];
		lL->len -= times[max];
	}else if(fabs(lL->pos[0])>1 || fabs(lL->pos[1])>1 || fabs(lL->pos[2])>1) return;//Starts past the viewing area
	for(i = 0; i < 3; i++){
		if(lL->dir[i] == 0){
			times[i] = HUGE_VAL;
			continue;
		}
		times[i] = (lL->dir[i]<0?-lL->pos[i]-1:1-lL->pos[i])/lL->dir[i];
	}
	max = times[1] < times[0];
	max = (times[max] < times[2]) ? max : 2;
	if(times[max] <= 0) return;//passes past the viewing area
	if(lL->len > times[max]){
		lL->len = times[max];
		//lL->major = lL->major & 1;//If we had to trim off the back, we're not going to guideline that vertex.
	}
	allocLine(who);
//	if(displayFlag) printf("(%f, %f, %f) <%f, %f, %f>\n", x, y, z, dx, dy, dz);
}

char isntInFrontOf(box* a, box *b, player *who){
	int i = 0;
	for(; i < 4; i++){
		double dpos = a->pos[i]-b->pos[i];
		if(fabs(dpos) < a->size[i]+b->size[i]) continue;
		if(dpos>0){
			if(who->pos[i] < a->pos[i]-a->size[i]) return 1;
		}else{
			if(who->pos[i] > a->pos[i]+a->size[i]) return 1;
		}
	}
	return 0;
}

void occlude(player *person){
	int i, j, m, n;
	box *who, *who2;
	for(m = 0; m < numBoxen; m++){
		who = boxen+m;
		for(n = 0; n < numBoxen; n++){
			if(n==m) continue;
			who2 = boxen+n;
			if(isntInFrontOf(who2, who, person)) continue;
			/*if(displayFlag){
				if(who2 == boxen){
//					printf("%d <%f %f %f> %f\n", who2->numFaces, who2->normals[0][0], who2->normals[0][1], who->normals[0][2], who2->facePositions[0]);
				}
			}*/
			for(i = 0; i < who->numLines; i++){
				line* what = who->lines+i;
				if(what->len == 0) continue;
				double IoDL=0, IoDU=what->len;//Interval of Death Lower and Upper, which describe the interval on which the exisiting line is going to be eclipsed by the current tesseract. Start off with the worst-case scenario, and trim from there.
				for(j = 0; j < who2->numFaces; j++){
					double pos = what->pos[0]*who2->normals[j][0] + what->pos[1]*who2->normals[j][1] + what->pos[2]*who2->normals[j][2];
					pos -= who2->facePositions[j];
					double dir = what->dir[0]*who2->normals[j][0] + what->dir[1]*who2->normals[j][1] + what->dir[2]*who2->normals[j][2];
					if(dir == 0){
						if(pos>0)
							break;
						continue;
					}
					double time = -pos/dir;
					if(dir>0){
						if(time < IoDU){
							IoDU = time;
							if(IoDU <= IoDL) break;
						}
					}else{
						if(time > IoDL){
							IoDL = time;
							if(IoDU <= IoDL) break;
						}
					}
				}
				if(j!=who2->numFaces) continue; //We broke out of the above loop, which means that the IoD is nothing.
				if(IoDL != 0 && IoDU != what->len){
					line* where = allocLine(who);
					memcpy(where, what, sizeof(line));
					for(j=0; j<3; j++){
						where->pos[j] += IoDU*what->dir[j];
					}
					where->len -= IoDU;
					what->len = IoDL;
				}else if(IoDL != 0){
					what->len = IoDL;
				}else if(IoDU != what->len){
					for(j=0; j<3; j++) what->pos[j] += what->dir[j] * IoDU;
					what->len -= IoDU;
				}else what->len = 0;
			}
#ifdef IND
			//Remove any indicators from who that are also present in who2
			i=0;
			while(i<who->numIndicators){
				int ind = who->indicators[i];
				for(j=0; j<who2->numIndicators; j++){
					if(who2->indicators[j]==ind){
						who->indicators[i] = who->indicators[--who->numIndicators];
						j=maxIndicators+1;
						break;
					}
				}
				if(j!=maxIndicators+1) i++;
			}
#endif
		}
	}
}

void render4Line(box *what, renderData *pt1, renderData *pt2) {
	if (!pt1->front){
		if (pt2->front) {
			renderData *tmp = pt1;
			pt1 = pt2;
			pt2 = tmp;
		} else
			return;
	}
	//what->color = i!=3?0xFF00FFFF:0xFF0000FF;
	if(pt2->front){
		addLine(what, pt1->pos[0], pt1->pos[1], pt1->pos[2], pt2->pos[0]-pt1->pos[0], pt2->pos[1]-pt1->pos[1], pt2->pos[2]-pt1->pos[2], what->color, 3);//major);
	}else{
		double dv[3];//In this section of code, 'v' is for value (one of {'x', 'y', 'z'})
		double remv[3];
		double adv[3];
		int m = 0;
		for(; m < 3; m++){
			dv[m] = pt1->pos[m]-pt2->pos[m];
			remv[m] = (dv[m]<0)?(pt1->pos[m]+1):(1-pt1->pos[m]);
			if(remv[m] <= 0) return;//offscreen, don't waste time
			adv[m] = fabs(dv[m]);
		}
		double vthing[3];
		for(m = 0; m < 3; m++){
			vthing[m] = remv[m]*adv[(m+1)%3]*adv[(m+2)%3];
		}
		int min = vthing[1] < vthing[0];
		min = (vthing[min] < vthing[2]) ? min : 2;
		double factor = remv[min]/adv[min];//set factor so that we get the line to the edge as cheaply as possible
		addLine(what, pt1->pos[0], pt1->pos[1], pt1->pos[2], factor*dv[0], factor*dv[1], factor*dv[2], what->color, 3);//major);
	}
}

void renderDetails(player *who, box *what, double *coords, int nvi /*non-varying index*/, double size) {
	if (coords[0]*coords[0] + coords[1]*coords[1] + coords[2]*coords[2] + coords[3]*coords[3] >= size*size * 4)
		return;
	uint32_t oldColor = what->color;
	what->color = (((oldColor & 0xFF000000) / 4 * 3) & 0xFF000000) +
		(((oldColor & 0xFF0000) / 4 * 3) & 0xFF0000) +
		(((oldColor & 0xFF00) / 4 * 3) & 0xFF00) + 0xFF;
	double tmp;
	renderData f1, f2;
	int i;
	for (i = 0; i < 4; i++) {
		if (i == nvi) continue;
		tmp = coords[i];
		coords[i] -= what->size[i];
		f1 = getRenderData(who, coords);
		coords[i] = tmp+what->size[i];
		f2 = getRenderData(who, coords);
		coords[i] = tmp;
		render4Line(what, &f1, &f2);
	}
	renderData ring[4];
	double tmp2;
	int j, k;
	for (i = 0; i < 3; i++) {
		if (i == nvi) continue;
		tmp = coords[i];
		for (j = i+1; j < 4; j++) {
			if (j == nvi) continue;
			tmp2 = coords[j];
			for (k = 0; k < 4; k++) {
				coords[i] = tmp + (primaryDim[k] ? what->size[i] : -what->size[i]);
				coords[j] = tmp2 + (secondaryDim[k] ? what->size[j] : -what->size[j]);
				ring[k] = getRenderData(who, coords);
			}
			coords[i] = tmp;
			coords[j] = tmp2;
			for (k = 0; k < 4; k++) {
				render4Line(what, ring + k, ring + (k + 1) % 4);
			}
		}
	}
	size /= 2;
	double tmp3;
	int ix1 = (nvi == 0) ? 1 : 0;
	int ix2 = (nvi < 2)  ? 2 : 1;
	int ix3 = (nvi == 3) ? 2 : 3;
	tmp = coords[ix1];
	tmp2 = coords[ix2];
	tmp3 = coords[ix3];
	int oldSizes[4];
	memcpy(oldSizes, what->size, sizeof(int)*4);
	what->size[ix1] /= 2;
	what->size[ix2] /= 2;
	what->size[ix3] /= 2;
	for (i = -1; i <= 1; i += 2) {
		coords[ix1] = tmp + size * i;
		for (j = -1; j <= 1; j += 2) {
			coords[ix2] = tmp2 + size * j;
			for (k = -1; k <= 1; k += 2) {
				coords[ix3] = tmp3 + size * k;
				renderDetails(who, what, coords, nvi, size);
			}
		}
	}
	what->color = oldColor;
	memcpy(what->size, oldSizes, sizeof(int)*4);
	coords[ix1] = tmp;
	coords[ix2] = tmp2;
	coords[ix3] = tmp3;
}

void render(player *who, box *what){
	//puts("");
#ifdef IND
	what->numIndicators = maxIndicators;
	memcpy(what->indicators, allIndicators, sizeof(int)*maxIndicators);
#endif
	double pos[4];//relative position of box's center from us
	int i = 0;
	for(; i < 4; i++){
		pos[i] = what->pos[i] - who->pos[i];
	}
	renderData vertexPoints[2][2][2][2];//viewing window coordinates of each corner
	int j, k, l;
	char visible[4];//Indicates which direction on each of the dimensions is definitely visible to the player. 0=back, 1=front, -1=neither. -1 serves as neither because the other two are used as indices
	for(i = 0; i < 4; i++){
		if(fabs(pos[i]) <= what->size[i])
			visible[i] = -1;
		else
			visible[i] = pos[i] > 0;
	}
	double vertexPos[4];//4-space coordinates of current corner
	for(i = 0; i < 2; i++){//calculating all of these is superfluous-- should trim down. (fixed?)
		vertexPos[0] = pos[0] + (i?-1:1)*what->size[0];
		for(j = 0; j < 2; j++){
			vertexPos[1] = pos[1] + (j?-1:1)*what->size[1];
			for(k = 0; k < 2; k++){
				vertexPos[2] = pos[2] + (k?-1:1)*what->size[2];
				for(l = 0; l < 2; l++){
					if(i!=visible[0] && j!=visible[1] && k!=visible[2] && l!=visible[3]){
						//save time and
						continue;
					}
					vertexPos[3] = pos[3] + (l?-1:1)*what->size[3];
					vertexPoints[i][j][k][l] = getRenderData(who, vertexPos);//, i&&j&&k);
					//if(i&&j&&k) printf("%d\t%d\t%s\n", vertexPoints[i][j][k].pos[0], vertexPoints[i][j][k].pos[1], vertexPoints[i][j][k].behind?"f":"b");
				}
			}
		}
	}
	unsigned char corner[4];//{0,1} coordinates of current corner
	unsigned char ix1, ix2, ix3;
	//char major; //This is for when you don't want to trace to truncated ends of lines
	renderData* pt1;
	renderData* pt2;
	double tmpPos; // Holds the value in pos which we're going to be modifying

	what->numFaces = 0;
	//For planes in 3-space which are bounding for a tesseract's projection, we need one of the non-planar indices to be visible and the other not. 'i' is the visible index in the below loop, and 'ix1' is the other. 'ix2' and 'ix3' are the planar indices, the ones we vary to produce the plane.
	for(i = 0; i < 4; i++){
		if(-1==visible[i]) continue;
		corner[i] = visible[i];

		tmpPos = pos[i];
		pos[i] += visible[i] ? -what->size[i] : what->size[i];
		renderDetails(who, what, pos, i, what->size[0]);
		pos[i] = tmpPos;

		for(ix1 = 0; ix1 < 4; ix1++){
			if(ix1==i) continue;
			ix2 = ix3 = 255;
			for(j=0; j<4; j++){//Assign 'ix2' and 'ix3'
				if(i!=j && ix1!=j){
					if(ix2==255) ix2 = j;
					else{
						ix3 = j;
						break;
					}
				}
			}
			for(j=0; j<2; j++){
				if(j==visible[ix1]) continue;
				corner[ix1] = j;
				for(k=0; k<4; k++){
					corner[ix2] = primaryDim[k];
					corner[ix3] = secondaryDim[k];
					if(vertexPoints[corner[0]][corner[1]][corner[2]][corner[3]].front) break;
				}
				if(k==4) continue;
				renderData* middle = vertexPoints[corner[0]][corner[1]][corner[2]] + corner[3];
				corner[ix2]=primaryDim[(k+1)%4];
				corner[ix3]=secondaryDim[(k+1)%4];
				pt1 = vertexPoints[corner[0]][corner[1]][corner[2]] + corner[3];
				corner[ix2]=primaryDim[(k+3)%4];
				corner[ix3]=secondaryDim[(k+3)%4];
				pt2 = vertexPoints[corner[0]][corner[1]][corner[2]] + corner[3];
				if((pt1->front != pt2->front) ^ ((ix2+ix3)%2) ^ j ^ visible[i] ^ (i>ix1)){
					renderData* tmp = pt1;
					pt1 = pt2;
					pt2 = tmp;
				}
				double dpos1[3], dpos2[3];
				for(k=0; k<3; k++){
					dpos1[k] = pt1->pos[k] - middle->pos[k];
					dpos2[k] = pt2->pos[k] - middle->pos[k];
					/*if(displayFlag && what==boxen){
						printf("%f, %f\n", dpos1[k], dpos2[k]);
					}*/
				}
				int face = what->numFaces;
				what->facePositions[face] = 0;
				for(k=0; k<3; k++){
					what->normals[face][k] = (dpos1[(k+1)%3]*dpos2[(k+2)%3])-(dpos1[(k+2)%3]*dpos2[(k+1)%3]);
					what->facePositions[face] += what->normals[face][k]*middle->pos[k];
				}
#ifdef IND
				//Trim out the indicators which are outside the image of the hyperbox
				l=0;
				double value;
				double *ind;
				while(l<what->numIndicators){
					value = 0;
					ind = realIndicators[what->indicators[l]];
					for(k=0; k<3; k++) value += what->normals[face][k]*ind[k];
					if(value>what->facePositions[face])
						what->indicators[l] = what->indicators[--what->numIndicators];
					else
						l++;
				}
#endif
				what->numFaces++;
			}
		}
	}
#ifdef IND
	if(what->numFaces == 0) what->numIndicators = 0; // This feels wrong...
#endif
	for(i = 0; i < 4; i++){//dimension of lines now being drawn
		ix1 = (i+1)%4;
		ix2 = (i+2)%4;
		ix3 = (i+3)%4;
		for(j = 0; j < 2; j++){
			corner[ix1] = j;
			for(k = 0; k < 2; k++){
				corner[ix2] = k;
				for(l = 0; l < 2; l++){
					int visibleDims = (j == visible[ix1]) + (k == visible[ix2]) + (l == visible[ix3]);
					if(visibleDims == 0) continue;
					corner[ix3] = l;
					corner[i] = 0;
					pt1 = vertexPoints[corner[0]][corner[1]][corner[2]] + corner[3];
					corner[i] = 1;
					pt2 = vertexPoints[corner[0]][corner[1]][corner[2]] + corner[3];
					render4Line(what, pt1, pt2);
				}
			}
		}
	}
}

/**
 * pos should be relative to player.
 */
static renderData getRenderData(player *who, double pos[4]){//, char verbose){
	renderData ret;
	double coords[4] = {0, 0, 0, 0};
	int i = 0;
	int j = 0;
	double dist;
	for(; i < 4; i++){//components
		dist = pos[i];
		for(j=0; j < 4; j++){//vectors
			coords[j] += dist * who->view[j][i];
		}
	}
	//if(coords[0] < 1) coords[0] = 1;
	ret.front = coords[0]>0;//?2:0;
	unsigned char max = 1+(fabs(coords[2])>fabs(coords[1]));
	max = (fabs(coords[max]) > fabs(coords[3])) ? max : 3;
	/*if(verbose){
		printf("%f\t%f\t%f\n", coords[max], coords[3-max], coords[0]);
	}*/
	if(fabs(coords[max])*0.9>100000*fabs(coords[0])){//deals with screen positions that may overflow 16-bit integer due to points very near our face.
		dist = (ret.front ? 100000.0 : -100000.0) / fabs(coords[max]);//dist is the factor to push the pt to the edge
		//printf("%f\t%f\t%f\n", coords[max], fabs(coords[max]), dist);
		for(i = 0; i < 3; i++){
			ret.pos[i] = dist*coords[i+1];
		}
	}else{
		dist = coords[0];//dist is coords[0]. Different use from above, same name for efficiency. Pointless efficiency, I know.
		for(i = 0; i < 3; i++){
			ret.pos[i] = 0.9*coords[i+1]/dist;
		}
	}
	return ret;
}
