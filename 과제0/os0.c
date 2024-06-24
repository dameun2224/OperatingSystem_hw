#include <stdio.h>
#include <stdlib.h>

// 프로세스 구조체
typedef struct {
    int pid;            // ID
    int arrival_time;   // 도착 시간
    int code_bytes;     // 코드 길이 (바이트)
} process;

// 코드 튜플 구조체
typedef struct {
    // unsigned char로 선언
    unsigned char operation;
    unsigned char length;
} code_tuple;

int main(int argc, char* argv[]) {
    process cur;
    code_tuple code;

    // 프로세스 읽어오기
    while(fread(&cur, sizeof(process), 1, stdin) == 1) {
        fprintf(stdout, "%d %d %d\n", cur.pid, cur.arrival_time, cur.code_bytes);
        
        // 코드 튜플 읽어오기
        for(int i=0; i< cur.code_bytes/2; i++) { // 튜플의 길이는 바이트 길이의 1/2배
            fread(&code, sizeof(code_tuple), 1, stdin);
            fprintf(stdout, "%d %d\n", code.operation, code.length);
        }

    }

    return 0;
}
