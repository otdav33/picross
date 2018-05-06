This is a web implementation of picross that does not use JavaScript. The goal
was to make a complete picross implementation in as few lines of code as 
possible.

Deployment is easy. You compile it with `gcc  -o picross *c picross.h` (or a
different compiler). You should get a "picross" executable file, which you can
run. You specify the port it will run on as the only argument, e.g.
`./picross 8080`. If you run it as root, it will drop privileges to group 32766,
user 32767, which is the user and group for "nobody" on OpenBSD. That might not
be the case for your system, so you can change DROP\_TO\_GROUP and
DROP\_TO\_USER in picross.h to something with minimal (or no) permissions. You
can also run this picross server as a user that is not root, and it will keep
the permissions of that user.

To play it, you can visit http://localhost:port/player?x=1&y=1&solution=w and
follow the instructions on the bottom of the page. Make sure to replace
"localhost:port" with the address and port of your server (For example, if you
are running it on your local machine on port 8080, it would be "localhost:8080".

If you want to make your own puzzles, just go to http://localhost:port/, again
replacing "localhost:port" with the address and port of your server. Right now,
it doesn't support HTTPS. You will be prompted to enter the width and height of
the playing board you want to make. You paint squares by clicking them. Once
you feel like your puzzle is good enough, click the link down below to play the
puzzle. You can copy that URL and give it to friends.
