#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define LIST_POISON1  ((void *) 0x00100100)
#define LIST_POISON2  ((void *) 0x00200200)

struct list_head {
	struct list_head* next, * prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head* new,
	struct list_head* prev,
	struct list_head* next) {
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head* new, struct list_head* head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head* prev, struct list_head* next) {
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}

static inline void list_del_init(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static inline void list_move(struct list_head* list, struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline void list_move_tail(struct list_head* list,
	struct list_head* head) {
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static inline int list_empty(const struct list_head* head) {
	return head->next == head;
}

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head);	\
       pos = pos->next)

#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; prefetch(pos->prev), pos != (head); \
        	pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head);					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.prev, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

#define list_for_each_entry_safe_reverse(pos, n, head, member)		\
	for (pos = list_entry((head)->prev, typeof(*pos), member),	\
		n = list_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.prev, typeof(*n), member))

#if 0    //DEBUG
#define debug(fmt, args...) fprintf(stderr, fmt, ##args)
#else
#define debug(fmt, args...)
#endif

/* ----------------- My Code ----------------- */

typedef struct {
	unsigned char operation;
	unsigned char length;
} code;

typedef struct {
	int pid;
	int arrival_time;
	int code_bytes;
	int program_counter;
	code* operations;
	struct list_head job, ready, wait;
} process;

int main(void) {
	process* cur, * next;
	process* idle;
	code codes;

	LIST_HEAD(job_q);
	LIST_HEAD(ready_q);
	LIST_HEAD(wait_q);



	/* 프로세스 정보 읽어오기 */
	while (1) {
		// 동적할당
		cur = malloc(sizeof(*cur));
		// 읽어올 데이터가 있다면
		if (fread(cur, sizeof(int), 3, stdin) == 3) { // int형 크기만큼 3번 읽어옴
			//fprintf(stdout, "%d %d %d\n", cur->pid, cur->arrival_time, cur->code_bytes);
			INIT_LIST_HEAD(&cur->job);
			INIT_LIST_HEAD(&cur->ready);
			INIT_LIST_HEAD(&cur->wait);
			cur->program_counter = 0;
			cur->operations = malloc(sizeof(code) * cur->code_bytes / 2);
			//printf("code_bytes : %d\n", cur->code_bytes);
			// 코드 튜플 읽어오기
			for (int i = 0; i < cur->code_bytes / 2; i++) { // 튜플의 길이는 바이트 길이의 1/2배
				fread(&codes, sizeof(code), 1, stdin);
				cur->operations[i] = codes;
			}
			list_add_tail(&cur->job, &job_q);
		}
		// 읽어올 데이터가 없다면
		else {
			free(cur);
			break;
		}
	}


	/* 프로세스 정보 출력하기 - 역순회 */
	/*
	list_for_each_entry_reverse(cur, &job_q, job) {
		printf("PID: %03d\tARRIVAL: %03d\tCODESIZE: %03d\n", cur->pid, cur->arrival_time, cur->code_bytes);
		code *cur_codes = cur->operations;
		for(int i=0; i<cur->code_bytes/2; i++) {
			printf("%d %d\n", cur_codes[i].operation, cur_codes[i].length);
		}
	}
	*/


	/* idle 프로세스 생성 */
	// pid = 100, operation = 0xFF
	idle = malloc(sizeof(*idle));
	idle->pid = 100;
	idle->arrival_time = 0;
	idle->code_bytes = 2;
	idle->program_counter = 0;
	idle->operations = malloc(sizeof(code));
	idle->operations[0].operation = 0xFF;
	idle->operations[0].length = 0;
	INIT_LIST_HEAD(&idle->job);
	INIT_LIST_HEAD(&idle->ready);
	INIT_LIST_HEAD(&idle->wait);
	list_add_tail(&idle->job, &job_q); // idle 프로세스를 jop queue 마지막에 추가


	/* 시뮬레이터 동작 */
	// program_counter : 0부터 n까지. 동작 상태: 0~n-1, 끝난 상태: n
	int idle_time = 0;
	int clock = 0;
	int cpu_clock = 0, io_clock = 0;
	process* run = idle;
	code run_code;

	while (1) {
		// 도착한 프로세스 확인 및 ready큐에 넣기
		list_for_each_entry(cur, &job_q, job) {
			if (clock == cur->arrival_time) {
				// idle이라면 ready queue 처음에, 아니라면 ready queue 마지막에 삽입
				if (cur->pid == 100) list_add(&cur->ready, &ready_q);
				else list_add_tail(&cur->ready, &ready_q);
				printf("%04d CPU: Loaded PID: %03d\tArrival: %03d\tCodesize: %03d\tPC: %03d\n", clock, cur->pid, cur->arrival_time, cur->code_bytes, cur->program_counter);
			}
		}

		if (clock == 0) {
			// 실행 중인 코드
			run = list_entry(ready_q.next, process, ready); // idle 프로세스
			// ready_q에 idle 프로세스만 있다면
			if (ready_q.prev != &idle->ready) run = list_entry(run->ready.next, process, ready); // idle 다음 프로세스
			run_code = run->operations[run->program_counter];
		}


		// 프로세스 및 코드 동작 종료 확인
		if (run_code.operation == 0) { // cpu 작업
			if (cpu_clock == run_code.length) {
				// increase PC 
				//printf("%04d CPU: increase PC\tPID: %03d\tPC: %04d\n", clock, run->pid, run->program_counter);
				run->program_counter++;
				run_code = run->operations[run->program_counter];
				cpu_clock = 0;
				// process 종료 확인
				if (run->program_counter == run->code_bytes / 2) {
					list_del(&run->ready);
					//printf("%04d CPU: Process is terminated PID: %03d PC: %04d\n", clock, run->pid, run->program_counter);
				}
			}
		}
		else if (run_code.operation == 1) { // io 작업
			if (run_code.length) { // io 작업 시작 
				list_add_tail(&run->wait, &wait_q); // wait_q 에 넣기
				list_del(&run->ready); // ready_q 에서 빼기
			}
			else {
				//printf("%04d CPU: increase PC\tPID: %03d\tPC: %04d\n", clock, run->pid, run->program_counter);
				run->program_counter++;
				run_code = run->operations[run->program_counter];
				if (run->program_counter == run->code_bytes / 2) {
					list_del(&run->ready);
					//printf("%04d CPU: Process is terminated PID: %03d PC: %04d\n", clock, run->pid, run->program_counter);
				}
			}
		}

		// 종료 조건 확인
		if (ready_q.prev == &idle->ready && list_empty(&wait_q)) { // ready_q에 idle만 남아있다면 종료
			break;
		}


		// 프로세스 및 코드 context swithcing
		process* nxt_process = list_entry(ready_q.next, process, ready); // idle 프로세스
		if (ready_q.prev != &idle->ready) nxt_process = list_entry(nxt_process->ready.next, process, ready); // idle 다음 프로세스

		if (run != nxt_process) { // process context swithcing
			//printf("%04d CPU: Reschedule\tPID: %03d\n", clock, run->pid);
			clock += 10;
			printf("%04d CPU: Switched\tfrom: %03d\tto: %03d\n", clock, run->pid, nxt_process->pid);
			// wait queue 작업 - -10씩 해주기
			if (!list_empty(&wait_q)) {
				process* io_process;
				process* nxt_io_process;
				for (int i = 0; i < 10; i++) {
					list_for_each_entry_safe(io_process, nxt_io_process, &wait_q, wait) {
						code* io_codes = io_process->operations;
						io_codes[io_process->program_counter].length--;
						if (!io_codes[io_process->program_counter].length) {
							printf("%04d IO : COMPLETED! PID: %03d\tIOTIME: %03d\tPC: %03d\n", clock, io_process->pid, clock - 10 + i, io_process->program_counter);
							list_add_tail(&io_process->ready, &ready_q); // ready_q 에 넣기
							list_del(&io_process->wait); // wait_q에서 빼기
						}
						//printf("%04d IO PID: %03d len: %03d\n", clock, io_process->pid, io_codes[io_process->program_counter].length);
					}
				}
			}
			idle_time += 10;
			run = nxt_process;
			run_code = run->operations[run->program_counter];

			if (run_code.operation == 1) {
				//printf("%04d CPU: increase PC\tPID: %03d\tPC: %04d\n", clock, run->pid, run->program_counter);
				run->program_counter++;
				run_code = run->operations[run->program_counter];
			}
		}
		else { // code context switching 혹은 아무것도 하지 않음 (같은 code)

		}

		if (run->program_counter == run->code_bytes / 2) {
			list_del(&run->ready);
			//printf("%04d CPU: Process is terminated PID: %03d PC: %04d\n", clock, run->pid, run->program_counter);
		}


		// 종료 조건 확인
		if (ready_q.prev == &idle->ready && list_empty(&wait_q)) { // ready_q에 idle만 남아있다면 종료
			if (run == idle) idle_time++;
			break;
		}


		// wait queue 작업
		if (!list_empty(&wait_q)) {
			process* io_process;
			process* nxt_io_process;
			list_for_each_entry_safe(io_process, nxt_io_process, &wait_q, wait) {
				code* io_codes = io_process->operations;
				io_codes[io_process->program_counter].length--; // length에 -1씩
				// length가 0이 됐다면 IO completed
				if (!io_codes[io_process->program_counter].length) {
					printf("%04d IO : COMPLETED! PID: %03d\tIOTIME: %03d\tPC: %03d\n", clock, io_process->pid, clock, io_process->program_counter);
					list_add_tail(&io_process->ready, &ready_q); // ready_q 에 넣기
					list_del(&io_process->wait); // wait_q에서 빼기
				}
				//printf("%04d IO PID: %03d len: %03d\n", clock, io_process->pid, io_codes[io_process->program_counter].length);
			}
		}


		// 다음 loop로 ()
		if (run_code.operation == 0) {
			//if(!cpu_clock) printf("%04d CPU: OP_CPU START len: %03d end at: %03d\n", clock, run_code.length, clock + run_code.length);
			cpu_clock++;
		}
		else if (run_code.operation == 1) {
			//printf("%04d CPU: OP_IO START len: %03d end at: %03d\n", clock, run_code.length, clock + run_code.length);
		}
		else if (run->pid) idle_time++;
		clock++;

		//printf("IDLE: %d\n", idle_time);
	}

	float util = (1 - ((float)idle_time / (float)clock)) * 100;
	printf("*** TOTAL CLOCKS: %04d IDLE: %04d UTIL: %2.2f%%\n", clock, idle_time, util);


	/* 동적할당 해제 */
	list_for_each_entry_safe(cur, next, &job_q, job) {
		list_del(&cur->job);
		free(cur->operations);
		free(cur);
	}

	return 0;
}