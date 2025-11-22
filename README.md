# any2any
a c program that let you convert anything (almost) in2 anything (almost) convert any file to a sendable txt that you can reconvert into the original file 

to build /compile it 
@user$ gcc -o any2any any2any.c

to convert the o'g file into txt (base64) 
@user$ ./any2any encode [your-o'g-file] [output-txt-file] --wrap 76

to convert the txt to the o'g file
@user$ ./any2any decode [ouput-txt-file] [your-o'g-file]

enjoy ;)

