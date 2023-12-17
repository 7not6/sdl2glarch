#include "common.h"

Coption *coreopt;
int nb_coreopt=0;
bool coreoptupdate=false;

void split(char *string,const char sep[],int ind){

	int i = 0;
	int taille,oldt;
	
	char *ptr = string;
	
	int nb_suboptval=0;
	
	while((ptr = strpbrk(ptr, sep)) != NULL){
    		nb_suboptval++; 
    		ptr++;    		
	}
	nb_suboptval++;
	
	coreopt[ind].sub.nb=nb_suboptval;
	coreopt[ind].sub.subopt=malloc(sizeof(char *)*nb_suboptval);
	coreopt[ind].sub.current=0;	
	
	i=0;
	ptr = string;
	taille=strlen(string);
	oldt=taille;
	
	while((ptr = strpbrk(ptr,sep)) != NULL){
	
		int t=strlen(ptr);
    		
    		coreopt[ind].sub.subopt[i]=malloc(sizeof(char)*MAX_OPTNAME_LENGTH); 
    		STRNCPY(coreopt[ind].sub.subopt[i],string+(taille-oldt)+(i==0?0:1),oldt-t+(i==0?1:0));	
    		oldt=t;    		
    		ptr++;
    		i++;
	}
	
	coreopt[ind].sub.subopt[i]=malloc(sizeof(char)*MAX_OPTNAME_LENGTH); 
	STRNCPY(coreopt[ind].sub.subopt[i],string+(taille-oldt)+1,oldt);
}

void freesuboptval(int ind){

	int nb_suboptval=coreopt[ind].sub.nb;

	for(int i=0;i<nb_suboptval;i++){
		//printf("%d)suboptval(%d)=!%s!\n",ind,i,coreopt[ind].sub.subopt[i]);
		free(coreopt[ind].sub.subopt[i]);		
	}
	
	free(coreopt[ind].sub.subopt);
}

