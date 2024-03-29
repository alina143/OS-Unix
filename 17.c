#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <termios.h>
#include <unistd.h>

#define LINE_SIZE 40

typedef struct
{
    char data[LINE_SIZE];
    int size;
} line_t;

void line_init(line_t *line)
{
    assert(NULL != line);
    
    line->size = 0;
}

char line_get(line_t *line, int index)
{
    assert(NULL != line);
    assert(index >= 0);
    assert(index < line->size);
    
    return line->data[index];
}

int line_get_size(line_t *line)
{
    assert(NULL != line);
    
    return line->size;
}

void line_append(line_t *line, char character)
{
    assert(NULL != line);
    assert(line->size < LINE_SIZE);
    
    write(STDOUT_FILENO, &character, 1);
    
    line->data[line->size] = character;
    line->size++;
}

void line_erase(line_t *line, int count)
{
    assert(NULL != line);
    assert(count >= 0);
    assert(count <= line->size);
    
    {
        int i = 0;
        for(i = 0;i < count;i++)
        {
            write(STDOUT_FILENO, "\b \b", 3);
            line->size--;
        }
    }
}

void line_erase_all(line_t *line)
{
    assert(NULL != line);
    
    line_erase(line, line->size);
}

void line_new_line(line_t *line)
{
    assert(NULL != line);
    
    write(STDOUT_FILENO, "\n", 1);
    line->size = 0;
}

void line_erase_word(line_t *line)
{
    assert(NULL != line);
    
    {
        int index = 0;
        for(index = line->size - 1;index >= 0;index--)
        {
            if(isspace(line_get(line, index)))
            {
                break;
            }
        }
        for(;index >= 0;index--)
        {
            if(!isspace(line_get(line, index)))
            {
                break;
            }
        }
        line_erase(line, line->size - index - 1);
    }
}

void line_wrap_word(line_t *line)
{
    assert(NULL != line);
    
    {
        int index = 0;
        for(index = line->size - 1;index >= 0;index--)
        {
            if(isspace(line_get(line, index)))
            {
                break;
            }
        }
        if(index >= 0)
        {
            int old_size = line->size;
            line_erase(line, line->size - index - 1);
            line_new_line(line);
            for(index++;index < old_size;index++)
            {
                line_append(line, line->data[index]);
            }
        }
        else
        {
            line_new_line(line);
        }
    }
}

int main(int argc, char **argv)
{
    if(isatty(STDIN_FILENO))
    {
        struct termios attributes;
        struct termios old_attributes;
        
        if(-1 != tcgetattr(STDIN_FILENO, &attributes))
        {
            old_attributes = attributes;
            
            attributes.c_lflag &= ~(ICANON | ECHO);
            attributes.c_cc[VMIN] = 1;
            if(-1 != tcsetattr(STDIN_FILENO, TCSAFLUSH, &attributes))
            {
                line_t line;
                line_init(&line);
                while(1)
                {
                    char character = '\0';
                    if(1 == read(STDIN_FILENO, &character, 1))
                    {
                        if(attributes.c_cc[VERASE] == character)
                        {
                            if(line_get_size(&line) > 0)
                            {
                                line_erase(&line, 1);
                            }
                        }
                        else if(attributes.c_cc[VKILL] == character)
                        {
                            line_erase_all(&line);
                        }
                        else if(attributes.c_cc[VWERASE] == character)
                        {
                            line_erase_word(&line);
                        }
                        else if(attributes.c_cc[VEOF] == character)
                        {
                            if(0 == line_get_size(&line))
                            {
                                break;
                            }
                        }
                        else if('\n' == character)
                        {
                            line_new_line(&line);
                        }
                        else if(isprint(character))
                        {
                            if(line_get_size(&line) == LINE_SIZE)
                            {
                                if(isspace(character))
                                {
                                    line_new_line(&line);
                                }
                                else
                                {
                                    line_wrap_word(&line);
                                }
                            }
                            line_append(&line, character);
                        }
                        else
                        {
                            write(STDOUT_FILENO, "\a", 1);
                        }
                    }
                    else
                    {
                        perror("Couldn't read from standard input");
                    }
                }
                
                if(-1 == tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_attributes))
                {
                    perror("Couldn't restore terminal attributes");
                }
            }
            else
            {
                perror("Couldn't set terminal attributes");
            }
        }
        else
        {
            perror("Couldn't get terminal attributes");
        }
    }
    else
    {
        fprintf(stderr, "Standard input is not a terminal");
    }
    return EXIT_SUCCESS;
}
