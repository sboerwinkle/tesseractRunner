typedef struct {
	double pos[3];
	char front;
} renderData;

extern int primaryDim[4];
extern int secondaryDim[4];

extern double viewerPos[3];
extern char headOnView;
extern double view3d[3][3];
extern double yaw3d;
extern double pitch3d;

extern void set3dViewVectors();

extern void initGraphics();

extern line viewSpaceBoundingLines[12];

extern void renderLineSingular(line* cur);

extern void renderLineGuides(line* cur);

extern void renderLines();

extern void occlude(player *person);

extern void render(player *who, box *what);

extern char displayFlag;
