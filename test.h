typedef struct{
	char major;//major lines get indicator lines
	double pos[3];
	int color;
	double dir[3];
	double len;
} line;

#define maxIndicators (indicatorDim*indicatorDim*indicatorDim)
#define indicatorDim 5

typedef struct box{
	int pos[4];
	int size[4];
	uint32_t color;

	line* lines;//Array of lines
	int lineCapacity;
	int numLines;

	double normals[12][3];
	double facePositions[12];
	int numFaces; //Only counts faces which are bounding in the 3d projection
	int indicators[maxIndicators];
	int numIndicators;
} box;

extern int numBoxen;
extern box* boxen;

extern box *addBox();

typedef struct {
	int pos[4];
	int vel[4];
	double desiredVel[3];
	double view[4][4];//Actual viewing directions: forward, right, up, 4th
	double moveVec[3];
	double yaw;
	int fitch;//fourth-dimensional pitch.
	double pitch;
	char canJump;
} player;

player me;
#define MAXFITCH 10
#define FITCHTOANGLE (M_PI_2/10)

#define playerSize 4000
