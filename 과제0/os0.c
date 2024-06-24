#include <stdio.h>
#include <stdlib.h>

// ���μ��� ����ü
typedef struct {
    int pid;            // ID
    int arrival_time;   // ���� �ð�
    int code_bytes;     // �ڵ� ���� (����Ʈ)
} process;

// �ڵ� Ʃ�� ����ü
typedef struct {
    // unsigned char�� ����
    unsigned char operation;
    unsigned char length;
} code_tuple;

int main(int argc, char* argv[]) {
    process cur;
    code_tuple code;

    // ���μ��� �о����
    while(fread(&cur, sizeof(process), 1, stdin) == 1) {
        fprintf(stdout, "%d %d %d\n", cur.pid, cur.arrival_time, cur.code_bytes);
        
        // �ڵ� Ʃ�� �о����
        for(int i=0; i< cur.code_bytes/2; i++) { // Ʃ���� ���̴� ����Ʈ ������ 1/2��
            fread(&code, sizeof(code_tuple), 1, stdin);
            fprintf(stdout, "%d %d\n", code.operation, code.length);
        }

    }

    return 0;
}
