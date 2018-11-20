#import "filesys.h"

int main()
{
   task_d();
   task_c();

   return 0;
}

void task_d(){
    format();
    writedisk("virtualdiskD3_D1");
}

void task_c(){
    MyFILE * new_text_file = myfopen("testfile.txt", "w");
    const char *my_text = " For the soviet union comrades. For the soviet struct comrades. ";

    for (int i = 0; i < TEXTFILESIZE; i++) {
        char current_char = my_text[i % strlen(my_text)];
        myfputc( (Byte) current_char, new_text_file);
    }

    myfclose(new_text_file);

    FILE * txt_file = fopen("testfileC3_C1_copy.txt", "w");

    MyFILE * read_file = myfopen("testfile.txt", "r");
    for (int c, i = 0; i < TEXTFILESIZE; i++) {
        c = myfgetc(read_file);
        if (i % LINELENGTH  == 0)
                  printf("\n");
        printf("%c", c);
        fputc(c, txt_file);
    }
    printf("\n");
    fclose(txt_file);
    myfclose(read_file);
    writedisk("virtualdiskC3_C1");
}