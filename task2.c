#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <elf.h>

char* fileName;
char* stringTable;
char *stringSHTable;
char *stringSYMTable;
int Currentfd=-1;
int numberOfsymbols=0;
Elf64_Ehdr *header; /* this will point to the header structure */
Elf64_Shdr *sectionTable; 
Elf64_Shdr *curr_sec_entry;
Elf64_Shdr *symbolTable; 
Elf64_Shdr *curr_sym_entry; 
Elf64_Sym *currSymbol; 
Elf64_Shdr *sectionSearch;
Elf64_Shdr *inSectionSearch;
Elf64_Shdr *symHeader;
Elf64_Sym  *symbolSearch;
void *map_start; /* will point to the start of the memory mapped file */
struct stat fd_stat; /* this is needed to  the size of the file */
unsigned char* magicBytes;
void clear();
void quit();
   	
void examine_elf_file() {
   	fileName = malloc (sizeof(char) * 1024);
	printf("Please enter file name\n");
	scanf("%s", fileName);
	if (Currentfd!=-1){
		close(Currentfd);
		clear();
	}
	if( (Currentfd = open(fileName, O_RDWR)) < 0 ) {
    	perror("error in open");
    	return;
   	}

   	if( fstat(Currentfd, &fd_stat) != 0 ) {
    	perror("stat failed");
      	clear();
    	quit();
   	}

   	if ( (map_start = mmap(0, fd_stat.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, Currentfd, 0)) == MAP_FAILED ) {
      	perror("mmap failed");
      	clear();
    	quit();
   	}
   	   /* now, the file is mapped starting at map_start.
    * all we need to do is tell *header to point at the same address:
    */

   	header = (Elf64_Ehdr *) map_start;
   	/* now we can do whatever we want with header!!!!
    * for example, first 3 bytes of magic */

   	magicBytes = malloc(4);
   	magicBytes[0] = header->e_ident[1];
   	magicBytes[1] = header->e_ident[2];
   	magicBytes[2] = header->e_ident[3];
   	magicBytes[3] = '\0';
   	printf("Bytes 1,2,3 of Magic: %s\n", magicBytes);
   	if (strcmp((char*)magicBytes, "ELF")!=0){
   		perror("not an elf file");
   		clear();
    	quit();
   	}

   	printf("The data encoding scheme: %x\n", header->e_ident[5]);
   	printf("Entry point: %x\n", (int)header->e_entry);
   	printf("The file offset in which the section header table resides: %d\n", (int)header->e_shoff);
   	printf("The number of section header entries: %d\n", header->e_shnum);
   	printf("The size of each section header entry: %d\n", header->e_shentsize);
   	printf("The file offset in which the program header table resides: %d\n", (int)header->e_phoff);
   	printf("The number of program header entries: %d\n", header->e_phnum);
   	printf("The size of each program header entry: %d\n", header->e_phentsize);

}


void print_section_names() {
	if(Currentfd == -1){
		printf("not an ELF file entered!\n");
		clear();
    	quit();
	}
	sectionTable = (Elf64_Shdr *)(map_start+(header->e_shoff)+(header->e_shstrndx)*(header->e_shentsize));
	stringTable = (char *)map_start+sectionTable->sh_offset;
	printf("[Nr] Name\t\t\tAddr\t off\tSize\tType\n");
	int i;
	for(i=0; i< header->e_shnum;i++){
		curr_sec_entry=(Elf64_Shdr *)(map_start+(header->e_shoff)+i*(header->e_shentsize));
		if(i==0)
			printf("[%d] %s \t\t\t\t%08lx %06lx %06lx  %d\n", i, (stringTable+curr_sec_entry->sh_name), curr_sec_entry->sh_addr, curr_sec_entry->sh_offset, curr_sec_entry->sh_size, curr_sec_entry->sh_type);
		else if(strlen((stringTable+curr_sec_entry->sh_name)) > 9)
			printf("[%d] %s \t\t%08lx %06lx %06lx  %d\n", i, (stringTable+curr_sec_entry->sh_name), curr_sec_entry->sh_addr, curr_sec_entry->sh_offset, curr_sec_entry->sh_size, curr_sec_entry->sh_type);
		else
			printf("[%d] %s \t\t\t%08lx %06lx %06lx  %d\n", i, (stringTable+curr_sec_entry->sh_name), curr_sec_entry->sh_addr, curr_sec_entry->sh_offset, curr_sec_entry->sh_size, curr_sec_entry->sh_type);
	}
	/*
	for (int k=0; k<header->e_shnum; k++){
		printf("%s\n", curr_sec_entry->sh_type);
	}*/
}

void print_symbols(){
	if( Currentfd == -1)
	{
		printf("not an ELF file entered!\n");
		clear();
    	quit();
	}
	
	sectionSearch = (Elf64_Shdr *)( map_start + ( header -> e_shoff )+
                                    ( header -> e_shstrndx ) * ( header -> e_shentsize ) );
	stringSHTable =  ( map_start + ( sectionSearch -> sh_offset ) );
	
	int i = 0;
    while (i < header->e_shnum)
    {
		sectionSearch = (Elf64_Shdr *)( map_start + ( header -> e_shoff ) +
            i * ( header -> e_shentsize ) );
	    if( ( sectionSearch -> sh_type ) == SHT_DYNSYM ) 
	    {               
	        symHeader = (Elf64_Shdr *)( map_start + ( header -> e_shoff ) +
	            ( sectionSearch -> sh_link ) * ( header -> e_shentsize ) );        
			stringSYMTable =  ( map_start + ( symHeader -> sh_offset ) );
			numberOfsymbols = sectionSearch -> sh_size / 24;            
	        if( numberOfsymbols == 0 )
			{
				printf("No symbols!\n");
				return;
			}            
	        printf("Symbol table '.dynsym' contains %d entries:\n" , numberOfsymbols );
			printf("[Num]\t Value\t\t Ndx\t Section\t\t Name\n");   
			int i;
			for(i = 0 ; i < numberOfsymbols ; i++ )
			{
				symbolSearch = (Elf64_Sym *)( map_start + ( sectionSearch -> sh_offset ) + i * 24 );
	                        inSectionSearch = (Elf64_Shdr *)( map_start + ( header -> e_shoff )+
						  ( symbolSearch -> st_shndx ) * ( header -> e_shentsize ) );
				printf("[%d]\t %08lx\t UND\t %s\t\t %s \n", i , symbolSearch -> st_value ,
						( stringSHTable + inSectionSearch -> sh_name ) ,
						( stringSYMTable + symbolSearch -> st_name ) ); }
	    }
		i++;
	}
    i=0;
    while (i < header->e_shnum)
	{
		sectionSearch = (Elf64_Shdr *)( map_start + ( header -> e_shoff ) +
                                                        i * ( header -> e_shentsize ) );
	       
		if( ( sectionSearch -> sh_type ) == SHT_SYMTAB )
        {
			numberOfsymbols = sectionSearch -> sh_size / 24;        
            symHeader = (Elf64_Shdr *)( map_start + ( header -> e_shoff ) +
            ( sectionSearch -> sh_link ) * ( header -> e_shentsize ) );        
            stringSYMTable =  ( map_start + ( symHeader -> sh_offset ) );        
            if( numberOfsymbols == 0 )
            {
                printf("No symbols!\n");
                return;
			}      
            printf("\nSymbol table '.symtab' contains %d entries:\n" , numberOfsymbols );
            printf("[Num]\t Value\t\t Ndx\t Section\t\t Name\n");
            int i;
            for(i = 0 ; i < numberOfsymbols ; i++ )
            {
                symbolSearch = (Elf64_Sym *)( map_start + ( sectionSearch -> sh_offset ) +
                                                        i * 24 );
                inSectionSearch = (Elf64_Shdr *)( map_start + ( header -> e_shoff )+
                                                ( symbolSearch -> st_shndx ) * ( header -> e_shentsize ) );
                        
                if( ( symbolSearch -> st_shndx <= header -> e_shnum -1 ) && ( symbolSearch -> st_shndx != 0) )
                {
                    printf("[%d]\t %08lx\t %d\t %s\t\t %s \n",
                          i , symbolSearch -> st_value , symbolSearch -> st_shndx ,
                          ( stringSHTable + inSectionSearch -> sh_name) , /*section name*/
                          ( stringSYMTable + symbolSearch -> st_name ) );           /*symbol name*/
				}
                else if( symbolSearch -> st_shndx == 0)
                {
                    printf("[%d]\t %08lx\t UND\t %s\t\t %s \n", i , symbolSearch -> st_value ,
                                                ( stringSHTable + inSectionSearch -> sh_name ) ,
                                                ( stringSYMTable + symbolSearch -> st_name ) );
                }		
				else
                {
                    printf("[%d]\t %08lx\t ABS\t ABS\t\t %s \n" , i , symbolSearch -> st_value ,
                                                        ( stringSYMTable + symbolSearch -> st_name ) );
				}
            }
        }
    i++;
    }       
}

void clear() {
	free (fileName);
	free (magicBytes);
	munmap(map_start, fd_stat.st_size);
}

void quit() {
    exit (0);
}


int main(int argc, char **argv) {
    void(*options[5]) ();
    options[1]=examine_elf_file;
    options[2]=print_section_names;
    options[3]=print_symbols;
    options[4]=quit;
    int choose;
    while(1){
        printf("%s", "Choose action:\n");
        printf("%s", "1-Examine ELF File\n");
        printf("%s", "2-Print Section Names\n");
        printf("%s", "3-Print Symbols\n");
        printf("%s", "4-Quit\n");
        scanf("%d", &choose);
        options[choose]();
    }
    
    return 0;
}