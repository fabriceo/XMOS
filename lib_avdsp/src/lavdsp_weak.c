
//placeholder for max 8 dsp tasks

void avdspTask1() __attribute__((weak));
void avdspTask1() {  };

void avdspTask2() __attribute__((weak));
void avdspTask2() {  };

void avdspTask3() __attribute__((weak));
void avdspTask3() {  };

void avdspTask4() __attribute__((weak));
void avdspTask4() {  };

void avdspTask5() __attribute__((weak));
void avdspTask5() {  };

void avdspTask6() __attribute__((weak));
void avdspTask6() {  };

void avdspTask7() __attribute__((weak));
void avdspTask7() {  };

void avdspTask8() __attribute__((weak));
void avdspTask8() {  };

int  avdspInit() __attribute__((weak));
int  avdspInit() { return 0; };

int  avdspSetProgram(unsigned prog) __attribute__((weak));
int  avdspSetProgram(unsigned prog) { return 0; };

int  avdspChangeFS(unsigned newFS) __attribute__((weak));
int  avdspChangeFS(unsigned newFS) { return 0; };

int avdspSetVolume(unsigned num, const int vol) __attribute__((weak));
int avdspSetVolume(unsigned num, const int vol) { return 0; }

int avdspSetBiquadCoefs(unsigned num, const float F, const float Q, const float G) __attribute__((weak));
int avdspSetBiquadCoefs(unsigned num, const float F, const float Q, const float G) { return 0;}

unsigned avdspGetChanend() __attribute__((weak));
unsigned avdspGetChanend() { return 0; };

void avdspTimeOverlap() __attribute__((weak));
void avdspTimeOverlap() { }
