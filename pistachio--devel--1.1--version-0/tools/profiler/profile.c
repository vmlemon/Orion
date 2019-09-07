#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

/*#define PROFILE_SIZE (98317)*/
//#define PROFILE_SIZE (196613)
//#define PROFILE_SIZE (393241)
//#define PROFILE_SIZE (786433)
#define PROFILE_SIZE (3145727)

#define INSTRUCTION_SIZE (4)
#define BUF_SIZE 128

struct function{
	struct function *next;
	char *name;
	uintptr_t start;
	uintptr_t length;
	float min_total;
	float avg_total;
	float max_total;
};

struct share{
	struct share *next;
	struct function *function;
	uintptr_t value;
};

unsigned long long profile_hits;

uintptr_t profile_table[PROFILE_SIZE];

uintptr_t share_entries[PROFILE_SIZE];
struct share *share_table[PROFILE_SIZE];

struct function *function_list = NULL;

unsigned long long hash(unsigned long long key){
	key *= 2654435761ull; /* knuth's magic number */
	return key;
}

void dump_share_entries(){
	int i;
	uintptr_t max=0, total=0, used_slots=0;

	printf("printing share table\n");

	for(i=0; i<PROFILE_SIZE; i++){
		total += share_entries[i];
		/*printf("%d = %lld\n", i, share_entries[i]);*/
		if(share_entries[i]>max){
			max = share_entries[i];
		}
		if(share_entries[i]>0){
			used_slots++;
		}
	}

	printf("hash conflicts: max=%d, avg=%f\n",
	       max, ((float)total)/((float)used_slots));
}

void build_function_table(FILE* f){
	uintptr_t ip, max, i;
	struct function *new, *entry;
	struct share *tmp;
	int r, good_fn=0, bad_fn=0;
	char buf[BUF_SIZE];
	int preload;

	/* zero the tables */
	for(i=0; i<PROFILE_SIZE; i++){
		share_entries[i]=0;
		share_table[i]=NULL;
	}

	new = malloc(sizeof(struct function));
	new->name = malloc(BUF_SIZE);

	/* fill in share_table and function_list */
	while(fgets(buf, BUF_SIZE, f)!=NULL){
		r=sscanf(buf,"%p%p%s",
	                 &new->start,
                         &new->length,
                         new->name);
preloaded:
		preload = 0;
		if(r!=3){
			bad_fn++;
			continue;
		}
		good_fn++;

		entry = new;
		if ((entry->length == 0) && (fgets(buf, BUF_SIZE, f)!=NULL))
		{
		    preload = 1;
		    new = malloc(sizeof(struct function));
		    new->name = malloc(BUF_SIZE);
		    r=sscanf(buf,"%p%p%s",
	                 &new->start,
                         &new->length,
                         new->name);

		    if(r==3){
			entry->length = new->start - entry->start;
			if (entry->length > 0x8000)
			    entry->length = 0;
		    }
		}
		entry->min_total = entry->avg_total = entry->max_total = 0.0;

		/* insert in function list */
		entry->next = function_list;
		function_list = entry;

		/* calculate shares */
		max = entry->start + entry->length;

		for(ip=entry->start; ip<max; ip+=INSTRUCTION_SIZE){
			i = hash(ip)%PROFILE_SIZE;
			share_entries[i]++;

			for(tmp=share_table[i]; tmp!=NULL; tmp=tmp->next){
				if(tmp->function==entry){
					tmp->value++;
					break;
				}
			}
			if(tmp==NULL){
				/* function not found - insert */
				tmp = malloc(sizeof(struct share));
				tmp->next = share_table[i];
				share_table[i] = tmp;
				tmp->function = entry;
				tmp->value = 1;
			}
		}

		if (preload)
		    goto preloaded;
		new = malloc(sizeof(struct function));
		new->name = malloc(BUF_SIZE);
	}

	free(new->name);
	free(new);

	printf("added %d functions to table\n", good_fn);
	printf("ignored %d functions\n", bad_fn);
}

void build_profile_table(FILE *f){
	unsigned long long i, ip;
	int r, good_i=0, bad_i=0;
	char buf[BUF_SIZE];

	profile_hits = 0;

	while(fgets(buf, BUF_SIZE, f)!=NULL){
		r=sscanf(buf,"%llx", &ip);
		if(r!=1){
			bad_i++;
			continue;
		}
		i = hash(ip)%PROFILE_SIZE;

//		if (profile_table[i])
		    profile_hits++;
//		if(share_entries[i] == 0)
//		    printf("missing %llx\n", ip);

		profile_table[i]++;

		good_i++;
	}

	printf("added %d profile_table entries\n", good_i);
	printf("ignored %d entries\n", bad_i);
	printf("total %lld profile hits\n", profile_hits);
}

void distribute_profile_hits(){
	struct share *s;
	uintptr_t missing = 0;
	float share_value;
	int i;

	for(i=0; i<PROFILE_SIZE; i++){
		if(profile_table[i] == 0)
			continue;

		if(share_entries[i] == 0){
			missing += profile_table[i];
			continue;
		}

		if(share_entries[i]>1){
			share_value = ((float)profile_table[i])
			            / ((float)share_entries[i]);

			printf("conflict:");
			for(s=share_table[i]; s!=NULL; s=s->next){
				printf(" \"%s\"", s->function->name);
				/* don't add to min_total if shared */
				s->function->avg_total +=
					(share_value * (float)s->value);
				s->function->max_total +=
					(float)profile_table[i];
			}
			printf("\n");
		}else{
			for(s=share_table[i]; s!=NULL; s=s->next){
				s->function->min_total +=
					(float)profile_table[i];
				s->function->avg_total +=
					(float)profile_table[i];
				s->function->max_total +=
					(float)profile_table[i];
			}
		}
	}
	printf("%ld missing entries = %2.2f%\n", missing, (float)missing/((double)profile_hits/100.0));
}

void dump_function_totals(){
	struct function *f;
	float total_hits;

	total_hits = ((double)profile_hits)/100.0;

	for(f=function_list; f!=NULL; f=f->next){
		if(f->max_total>0){
			if(f->min_total==0.0){
				printf("?");
			}else{
				printf(" ");
			}
			printf("%2.2f %2.2f %2.2f %s (%lx)\n", 
			       f->min_total/total_hits,
			       f->avg_total/total_hits,
			       f->max_total/total_hits,
			       f->name, f->start);
		}
	}
}

int main(int argc, char *argv[]){
	FILE *functions, *data;

	if(argc!=3){
		printf("usage: profile <%s> <%s>\n",
		       "function table", "data");
		return -1;
	}

	functions = fopen(argv[1], "r");
	if(functions==NULL){
		perror("Error opening function table");
		return -1;
	}

	data = fopen(argv[2], "r");
	if(data==NULL){
		perror("Error opening data");
		return -1;
	}

	build_function_table(functions);

	build_profile_table(data);

	distribute_profile_hits();

	dump_function_totals();

	dump_share_entries();

	return 0;
}
