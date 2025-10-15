 

// TEST.h
#ifndef TEST_H
#define TEST_H

// Envuelve las declaraciones en extern "C" si se incluye desde C++
#ifdef __cplusplus
extern "C" {
#endif
	
#define NUM 987654321
unsigned int  getNum(void);

#ifdef __cplusplus
}
#endif

#endif // TEST_H