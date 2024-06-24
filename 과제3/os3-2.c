#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define PAGESIZE (32)
#define PAS_FRAMES (256) //fit for unsigned char frame in PTE
#define PAS_SIZE (PAGESIZE*PAS_FRAMES) //32*256 = 8192 B
#define VAS_PAGES (64)
#define VAS_SIZE (PAGESIZE*VAS_PAGES) //32*64 = 2048 B
#define PTE_SIZE (4) //sizeof(pte)
#define PAGETABLE_FRAMES (VAS_PAGES*PTE_SIZE/PAGESIZE) //64*4/32 = 8 consecutive frames
#define PAGE_INVALID (0)
#define PAGE_VALID (1)
#define MAX_REFERENCES (256)
#define MAX_PROCESS (10)

typedef struct{
int pid;
int ref_len; //Less than 255
unsigned char *references;
} process_raw;

typedef struct{
unsigned char frame; //allocated frame
unsigned char vflag; //valid-invalid bit
unsigned char ref; //reference bit
unsigned char pad; //padding
} pte; // Page Table Entry (total 4 Bytes, always)

typedef struct {
unsigned char b[PAGESIZE];
} frame;

typedef struct{
    process_raw *process;
    int ref_idx;
    int page_fault_num;
} pm;

int process_num = 0;
pm *process_arr;
frame *pas;
pte *page_table[MAX_PROCESS];
int free_frame = 0;

/* 메모리 할당 */
void init_pas() {
    pas = (frame *)malloc(PAS_SIZE);
    process_arr = (pm *)malloc(MAX_PROCESS * sizeof(pm));
}

/* page table 초기화 */
bool init_pageTable() {
    // out of memory 확인
    if(free_frame >= PAS_FRAMES) return false;
    // 1 frame, 8 pte 할당
    pte *cur_pte = (pte *) &pas[free_frame];
    for(int i=0; i<PAGETABLE_FRAMES; i++) {
        cur_pte[i].frame = -1;
        cur_pte[i].vflag = PAGE_INVALID;
        cur_pte[i].ref = 0;
        cur_pte[i].pad = 0;
    }
    return true;
}

/* page_fault 핸들러 */ 
bool page_fault_handler(pte *cur_pte,int pte_idx) {
    // out of memory 확인
    if(free_frame >= PAS_FRAMES) return false;
    cur_pte[pte_idx].frame = free_frame;
    cur_pte[pte_idx].vflag = PAGE_VALID;
    cur_pte[pte_idx].ref = 1;
    free_frame++;
    return true;
}

/* 프로세스 정보 읽어오기 */
void load_process() {
    //printf("load_process() start\n");
    process_raw *cur;
    while(1) {
        cur = malloc(sizeof(*cur));
        if(fread(cur, sizeof(int) * 2, 1, stdin) == 1) {
            /* 프로세스 정보 읽어오기 */ 
            // pid, ref_len 정보 읽어오기
            cur->references = malloc(cur->ref_len);
            //printf("%d %d\n", cur->pid, cur->ref_len);
            // references 정보 읽어오기
            for(int i=0; i<cur->ref_len; i++) { 
                fread(&cur->references[i], sizeof(unsigned char), 1, stdin);
                //printf("%d ", cur->references[i]);
            }
            //printf("\n");
            /* page_table 초기화 */
            init_pageTable();
            free_frame++;
            /* pm 초기화 */
            process_arr[cur->pid].process = cur;
            process_arr[cur->pid].ref_idx = 0;
            process_arr[cur->pid].page_fault_num = 0;
            process_num++;
            
        }
        else {
            free(cur);
            break;
        }
    }
    //printf("load_process() end\n");
}

/* Demand Paging  with 2-level Hierarchical Page Table 수행 */
void start() {
    //printf("Start() start\n");
    while(1) {
        bool flag = true; // 종료 조건 확인 변수

        for(int i=0; i<process_num; i++) {
            process_raw *cur = process_arr[i].process;
            int idx = process_arr[i].ref_idx;

            // 레퍼런스를 모두 봤다면 건너뜀
            if(idx >= cur->ref_len) continue;
            flag = false;

            //printf("[PID %02d REF: %03d] Page access %03d: ", cur->pid, idx, cur->references[idx]);
            
            /* L1PT 확인 */
            pte *l1pt = (pte *)&pas[cur->pid];
            int l1pt_idx = cur->references[idx] / PAGETABLE_FRAMES;
            // page_fault인 경우
            if(l1pt[l1pt_idx].vflag == PAGE_INVALID) {
                if(!init_pageTable()) { // 페이지 테이블 추가
                    printf("Out of memory!!\n");
                    return;
                }
                if(!page_fault_handler(l1pt, l1pt_idx)) { // page_fault 핸들러
                    printf("Out of memory!!\n");
                    return;
                }
                process_arr[i].page_fault_num++;
                //printf("(L1PT) PF,Allocated Frame %03d -> %03d,", l1pt_idx, l1pt[l1pt_idx].frame);
            } 
            else {
                l1pt[l1pt_idx].ref++;
                //printf("(L1PT) Frame %03d,", l1pt[l1pt_idx].frame);
            }

            /* L2PT 확인 */
            pte *l2pt = (pte *) &pas[l1pt[l1pt_idx].frame];
            int l2pt_idx = cur->references[idx] % PAGETABLE_FRAMES;
            // page_fault인 경우
            if(l2pt[l2pt_idx].vflag == PAGE_INVALID) {
                if(!page_fault_handler(l2pt, l2pt_idx)) { // page_fault 핸들러
                    printf("Out of memory!!\n");
                    return;
                }
                process_arr[i].page_fault_num++;
                //printf("(L2PT) PF,Allocated Frame %03d\n", l2pt[l2pt_idx].frame);
            }
            else {
                l2pt[l2pt_idx].ref++;
                //printf("(L2PT) Frame %03d\n", l2pt[l2pt_idx].frame);
            }
            process_arr[i].ref_idx++;
        }
        if(flag) break;
    }
    //printf("Start() end\n");
}

/* 결과 출력 */
void print_result() {
    // 결과 출력용 변수
    int total_allocated_frame = 0;
    int total_page_fault = 0;
    int total_references = 0;

    for(int i=0; i<process_num; i++) { // 프로세스
        process_raw *cur = process_arr[i].process;
        int page_fault = process_arr[i].page_fault_num;
        int refer_num = process_arr[i].ref_idx;

        printf("** Process %03d: Allocated Frames=%03d PageFaults/References=%03d/%03d\n", i, page_fault+1, page_fault, refer_num);
        
        /* L1 PT */
        pte *l1pt = (pte *) &pas[cur->pid];
        for(int j=0; j<PAGETABLE_FRAMES; j++) {
            if(l1pt[j].vflag == PAGE_INVALID) continue; // 조건 확인
            printf("(L1PT) %03d -> %03d\n", j, l1pt[j].frame);

            /* L2 PT */
            pte *l2pt = (pte *) &pas[l1pt[j].frame];
            for(int k=0; k<PAGETABLE_FRAMES; k++) {
                if(l2pt[k].vflag == PAGE_INVALID) continue; // 조건 확인
                printf("(L2PT) %03d -> %03d REF=%03d\n", j*8+k, l2pt[k].frame, l2pt[k].ref);
            }
        }

        total_allocated_frame += (page_fault + 1);
        total_page_fault += page_fault;
        total_references += refer_num;
    }
    
    printf("Total: Allocated Frames=%03d Page Faults/References=%03d/%03d\n", total_allocated_frame, total_page_fault, total_references);
}

/* 동적 메모리 해제 */
void free_memory() {
    for(int i=0; i<process_num; i++) {
        free(process_arr[i].process->references);
        free(process_arr[i].process);
    }
    free(process_arr);
    free(pas);
}

int main(void){

    init_pas();     // 초기 메모리 할당
    load_process(); // 프로세스 정보 읽어오기
    start();        // Demand Paging
    print_result(); // 결과 출력
    free_memory();  // 메모리 해제

    return 0;
}