extern void c64_Init(void);
extern void c64_Step(void);
extern void c64_Start(char * filename);
extern void c64_Input(int key);

#define matrixMask(i) (((1 << (i % 8)) << 8) | (1 << (i / 8)));