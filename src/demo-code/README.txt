The 'za-' prefix for the .c files with main() function 
does not mean anything --- it's an arbitrary choice. 
It seemed suitable because on my GNU/Linux system 
there were no commands beginning with 'za'. 
I added the dash for readability, not for uniqueness.

These programs and utility modules are intended for educational purpose, 
not production use.

Most important element that makes them unsuitable for production use 
is the error handling:
    (1) We at least try to report most errors --- this is about the only
          recommended practice (that would be relevant in production code).
    (2) Sometimes there are unnecesary explanations in the error messages,
          or too verbose explanations --- for educational purpose.
    (3) But exiting after an error would not be acceptable in most cases
          in production code.
