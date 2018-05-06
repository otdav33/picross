#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include "picross.h"

void _stradd(char **buf, size_t *bufend, size_t *buflen,
        const char *addition, size_t addlen) {
    // concatenates the string addition (with length addlen) into *buf
    // buf has text length *bufend and capacity *buflen
    if (addlen + *bufend > *buflen)
        *buf = realloc(*buf, *buflen += addlen + BUFSTEP);
    strlcpy(*buf + *bufend, addition, *buflen - *bufend);
    *bufend += addlen - 1;
}

char *fillboard(char *board, int boardsize, char fillchar) {
    /*removes invalid characters (chars not 'b' or 'w'), and replace them with
      fillchar, and if the board is too small, resize it, and fill the rest with
      fillchar. fillboard returns board. board may be realloc'd, you you must
      reassign board to the return value.*/
    int i;
    if (board == 0) {
        board = malloc(boardsize + 1);
        board[0] = '\0';
    } else {
        board = realloc(board, boardsize + 1);
    }
    int bsz = strnlen(board, boardsize);
    for (i = 0; i < bsz; i++) {
        if ((board[i] != 'b') && (board[i] != 'w')) {
            board[i] = fillchar;
        }
    }
    for (i = bsz; i < boardsize; i++) board[i] = fillchar;
    board[boardsize] = '\0';
    return board;
}

int genborder(long width, long height, int is_top, char *board, int **border) {
    /*generate a border on either the top or left side of the board (just the
      numbers, not the string output), and put it in border.
      returns the border width or height*/
    int i, j, bordersize;
    for (i = 0; i < (is_top ? width : height); i++) {
        int strips = 0; //number of black strips
        int l = 0; //number of squares in strip
        for (j = 0; j < (is_top ? height : width); j++) {
            if (board[(is_top ? i : j) + ((is_top ? j : i) * width)] == 'b') {
                l++;
            } else if (l != 0) {
                border[i][++strips] = l;
                l = 0;
            }
        }
        if (l != 0) {
            border[i][++strips] = l;
            l = 0;
        }
        border[i][0] = strips;
        bordersize = bordersize > strips ? bordersize : strips;
    }
    if (bordersize <= 0) bordersize = 1;
    return bordersize;
}

int have_won(long width, long height, int **tborder, int **lborder,
        char *board) {
    //check to see if you won

    int ntw = ((height / 2) + 2);
    int nlw = ((width / 2) + 2);
#ifdef DEBUG
    int maxallowed = ((25 / 2) + 2);
    if (ntw > maxallowed || nlw > maxallowed) {
        printf("Error! ntw or nlw is too big!\n"
            "ntw is %i\n"
            "nlw is %i\n"
            "height is %i\n"
            "width is %i\n", ntw, nlw, height, width);
    }
#endif
    int i, j, retval = 0, *ntborder[width], *nlborder[height];
    ntborder[0] = alloca(ntw * width * sizeof(int));
    nlborder[0] = alloca(nlw * height * sizeof(int));
#ifdef DEBUG
    printf("Allocating ntborder as %i * %i = %i\n", ntw, width, ntw * width);
    printf("Allocating nlborder as %i * %i = %i\n", nlw, width, nlw * width);
#endif
    for (i = 1; i < width; i++)
        ntborder[i] = ntborder[0] + ntw * i;
    for (i = 0; i < height; i++)
        nlborder[i] = nlborder[0] + nlw * i;
    /*ntborder and nlborder are (or will be) what the borders would look
      like if they were generated from what is on the board right now.
      If they match the current borders, then the puzzle is solved.*/

    genborder(width, height, 1, board, ntborder);
    for (i = 0; i < width; i++) {
        int l = tborder[i][0];
        if (l != ntborder[i][0])
            goto not_won;
        for (j = 0; j <= l; j++) {
            if (tborder[i][j] != ntborder[i][j])
                goto not_won;
        }
    }

    genborder(width, height, 0, board, nlborder);
    for (i = 0; i < height; i++) {
        int l = lborder[i][0];
        if (l != nlborder[i][0])
            goto not_won;
        for (j = 0; j <= l; j++) {
            if (lborder[i][j] != nlborder[i][j])
                goto not_won;
        }
    }

    //If we got this far, we must have won
    retval = 1;

not_won:
    /*
    free(ntborder[0]);
    free(nlborder[0]);*/

    return retval;
}

void makeboard(long width, long height, char *board, int boardsize,
        int is_editor, char *solution, char **buffer, size_t *bufend, size_t *buflen) {
    char open_table[] = "<table>\n";
    char close_table[] = "</table>\n";
    char you_won[] = "<h3>You Won!</h3>\n"
        "<p>(or, at least you have a possible solution) The intended solution"
        " may have been different, as there are certain puzzles with multiple"
        " solutions. Here is what it was supposed to look like:</p>\n";
    char open_tr[] = "<tr>\n";
    char close_tr[] = "</tr>\n";
    char empty_th[] = "<th></th>\n";
    char zero_th[] = "<th>0</th>\n";
    char question_td[] = "<td>?</td>\n";
    char and_board_equals[] = "&board=";
    char td_class_b_editor[] = "<td class=\"b\"><a href=\"editor?%s\">&#x25A0;</a></td>\n";
    char td_class_w_editor[] = "<td class=\"w\"><a href=\"editor?%s\">&nbsp;&nbsp;</a></td>\n";
    char td_class_u_editor[] = "<td class=\"u\"><a href=\"editor?%s\">&nbsp;&nbsp;</a></td>\n";
    char td_class_b_player[] = "<td class=\"b\"><a href=\"player?%s\">&#x25A0;</a></td>\n";
    char td_class_w_player[] = "<td class=\"w\"><a href=\"player?%s\">&nbsp;&nbsp;</a></td>\n";
    char td_class_u_player[] = "<td class=\"u\"><a href=\"player?%s\">&nbsp;&nbsp;</a></td>\n";
    char td_class_b_plain[] = "<td class=\"b\">&#x25A0;</td>\n";
    char td_class_w_plain[] = "<td class=\"w\">&nbsp;&nbsp;</td>\n";
    char td_class_u_plain[] = "<td class=\"u\">&nbsp;&nbsp;</td>\n";
    int i, j, errcode;
    char temp[30];
    char *linkquery, *lqboard, *td_tmp;

    //make the output
    stradd(*buffer, *bufend, *buflen, open_table, sizeof(open_table));
    int *tborder[width];
    for (i = 0; i < width; i++)
        tborder[i] = calloc((height / 2) + 2, sizeof(int));
    //tborder[column #][0] = height of column
    //tborder[starts at 0, ends at  width][starts at 1, ends at tborder[col][0]]
    int *lborder[height];
    for (i = 0; i < height; i++)
        lborder[i] = calloc((width / 2) + 2, sizeof(int));
    //lborder[row #   ][0] = height of row
    //lborder[starts at 0, ends at height][starts at 1, ends at lborder[row][0]]
    //Other values in tborder and lborder are the actual values that show up on
    //the final picross border.

    char *board_determiner = is_editor ? board : solution;

    //generate top border, but don't add it to output yet
    int borderheight = genborder(width, height, 1, board_determiner, tborder);
    //generate left border, but don't add it to output yet
    int borderwidth  = genborder(width, height, 0, board_determiner, lborder);

    //make top border
    for (i = 0; i < borderheight; i++) {
        stradd(*buffer, *bufend, *buflen, open_tr, sizeof(open_tr));
        //upper left corner
        for (j = 0; j < borderwidth; j++)
            stradd(*buffer, *bufend, *buflen, empty_th, sizeof(empty_th));
        //main part
        for (j = 0; j < width; j++) {
            int ind = 1 + i - (borderheight - tborder[j][0]);
            if (ind <= 0) {
                if (ind == 0 && tborder[j][0] == 0)
                    stradd(*buffer, *bufend, *buflen, zero_th, sizeof(zero_th));
                else
                    stradd(*buffer, *bufend, *buflen, empty_th, sizeof(empty_th));
            } else {
                errcode = snprintf(temp, sizeof(temp), "<th>%i</th>\n",
                        tborder[j][ind]);
                if (errcode < 0 || errcode > sizeof(temp)) {
                    stradd(*buffer, *bufend, *buflen, question_td,
                            sizeof(question_td));
                    printf("error %i for snprintf #3\n", errcode);
                } else {
                    stradd(*buffer, *bufend, *buflen, temp,
                            strnlen(temp, sizeof(temp)) + 1);
                }
            }
        }
        stradd(*buffer, *bufend, *buflen, close_tr, sizeof(close_tr));
    }

    //make main part
    int lqsize;
    if (is_editor) {
        lqsize = boardsize + 50;
        linkquery = malloc(lqsize); //should be long enough
        errcode = snprintf(linkquery, lqsize, "x=%i&y=%i&board=%s", width,
                height, board);
        if (errcode < 0 || errcode > lqsize)
            printf("error %i for snprintf #4\n", errcode);
    } else {
        lqsize = (2 * boardsize) + 60;
        linkquery = malloc(lqsize); //should be long enough
        errcode = snprintf(linkquery, lqsize, "x=%i&y=%i&board=%s&solution=%s",
                width, height, board, solution);
        if (errcode < 0 || errcode > lqsize)
            printf("error %i for snprintf #5\n", errcode);
    }

    int tdtsize = lqsize + sizeof(td_class_b_editor);
    td_tmp = malloc(tdtsize);
    lqboard = strstr(linkquery, and_board_equals)
        + sizeof(and_board_equals) - 1;
    for (i = 0; i < height; i++) {
        stradd(*buffer, *bufend, *buflen, open_tr, sizeof(open_tr));
        //make left border
        for (j = 0; j < borderwidth; j++) {
            int ind = 1 + j - (borderwidth - lborder[i][0]);
            if (ind <= 0) {
                if (ind == 0 && lborder[i][0] == 0)
                    stradd(*buffer, *bufend, *buflen, zero_th, sizeof(zero_th));
                else
                    stradd(*buffer, *bufend, *buflen, empty_th, sizeof(empty_th));
            } else {
                errcode = snprintf(temp, sizeof(temp), "<th>%i</th>\n",
                        lborder[i][ind]);
                if (errcode < 0 || errcode > sizeof(temp)) {
                    stradd(*buffer, *bufend, *buflen, question_td,
                            sizeof(question_td));
                    printf("error %i for snprintf #6 \n", errcode);
                } else {
                    stradd(*buffer, *bufend, *buflen, temp,
                            strnlen(temp, sizeof(temp)) + 1);
                }
            }
        }
        //main part
        for (j = 0; j < width; j++) {
            char c = board[i * width + j];
            char *td_template;
            if (is_editor) {
                if (c == 'b') {
                    lqboard[i * width + j] = 'w';
                    td_template = td_class_b_editor;
                } else if (c == 'w') {
                    lqboard[i * width + j] = 'b';
                    td_template = td_class_w_editor;
                } else {
                    lqboard[i * width + j] = 'b';
                    td_template = td_class_u_editor;
                }
            } else { 
                if (c == 'b') {
                    lqboard[i * width + j] = 'w';
                    td_template = td_class_b_player;
                } else if (c == 'w') {
                    lqboard[i * width + j] = 'u';
                    td_template = td_class_w_player;
                } else {
                    lqboard[i * width + j] = 'b';
                    td_template = td_class_u_player;
                }
            }
            errcode = snprintf(td_tmp, tdtsize, td_template, linkquery);
            if (errcode < 0 || errcode > tdtsize) {
                stradd(*buffer, *bufend, *buflen, question_td, sizeof(question_td));
                printf("error %i for snprintf #7 \n", errcode);
            } else {
                stradd(*buffer, *bufend, *buflen, td_tmp,
                        strnlen(td_tmp, tdtsize) + 1);
            }
            lqboard[i * width + j] = c;
        }
        stradd(*buffer, *bufend, *buflen, close_tr, sizeof(close_tr));
    }
    free(linkquery);
    stradd(*buffer, *bufend, *buflen, close_table, sizeof(close_table));

    //If we won, let's output the "you win" message.
    if ((!is_editor) && have_won(width, height, tborder, lborder, board)) {
        stradd(*buffer, *bufend, *buflen, you_won, sizeof(you_won));
        stradd(*buffer, *bufend, *buflen, open_table, sizeof(open_table));
        for (i = 0; i < height; i++) {
            stradd(*buffer, *bufend, *buflen, open_tr, sizeof(open_tr));
            for (j = 0; j < width; j++) {
                if (solution[i * width + j] == 'b') {
                    stradd(*buffer, *bufend, *buflen, td_class_b_plain,
                            sizeof(td_class_b_plain));
                } else {
                    stradd(*buffer, *bufend, *buflen, td_class_w_plain,
                            sizeof(td_class_w_plain));
                }
            }
            stradd(*buffer, *bufend, *buflen, close_tr, sizeof(close_tr));
        }
        stradd(*buffer, *bufend, *buflen, close_table, sizeof(close_table));
    }

    for (i = 0; i < width; i++)
        free(tborder[i]);
    for (i = 0; i < height; i++)
        free(lborder[i]);
}

char *generate(char *query, char *path, int *final_length) {
    /*will return a malloc'd string (that will need to be freed later) that
      has html generated for either the picross editor or player (depending on
      the path).
      final_length will be set to the length of the returned string.*/
    size_t bufend = 0, buflen = BUFSTART;
    char *buf = malloc(buflen);
    char start[] =  "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset=\"utf-8\" />\n"
        "<title>Picross</title>\n"
        "<style>\n"
        "body {\n"
        "  background: #ffecb3\n"
        "}\n"
        "a {\n"
        "  text-decoration: none;\n"
        "  color: black;\n"
        "}\n"
        ".helper {\n"
        "  text-decoration: underline;\n"
        "}\n"
        "table {\n"
        "  text-align: center;\n"
        "  border-collapse: collapse;\n"
        "}\n"
        "th {\n"
        "  width: 20px;\n"
        "  height: 20px;\n"
        "  border: none;\n"
        "  font-weight:normal;\n"
        "  color: black;\n"
        "}\n"
        "td {\n"
        "  width: 20px;\n"
        "  height: 20px;\n"
        "  border: 1px solid #000000;\n"
        "  color: black;\n"
        "}\n"
        ".b {\n"
        "  background: black;\n"
        "}\n"
        ".w {\n"
        "  background: white;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n";
    char picross_editor[] = "<h3>Picross Editor</h3>\n";
    char picross_player[] = "<h3>Picross</h3>\n";
    long width, height;
    int errcode;
    char temp[30];
    char *endptr;
    char end_bit[] = "</p></body></html>\n\n";
    char play_editing_game[] = "<p><a href=\"player?x=%i&y=%i&solution=%s\">"
        "Play this picross</a></p>\n";
    char editor_help[] = "<div style=\"border: 3px solid #000000; margin: 20px;"
        " padding: 20px\">\n<br><h3>Instructions</h3>\n<p>Click the tiles until"
        " you get the puzzle looking how you want, then click the \"Play this"
        " picross\" button.</p>\n</div>\n";
    char player_help[] = "<div style=\"border: 3px solid #000000; margin: 20px;"
        " padding: 20px\">\n<h3>Instructions</h3>\n<p>The numbers above and to"
        " the left of the grid indicate the number of boxes that need to be"
        " shaded. There should be adjacent black boxes that form lines of the"
        " length indicated by those boxes. There needs to be at least one"
        " unshaded box between each line of black boxes, or you have one line,"
        " not two. You can also have any number of unshaded boxes at the"
        " beginning or end of your row or column.</p>\n"
        "<p>To shade boxes, click them. You can click the boxes again to mark"
        " them white (which counts as being unshaded), and then again to get"
        " them to their original cream-colored unshaded state.</p>\n"
        "<p>That might be a bit confusing, so if you're just starting out, try"
        " these example puzzles to get up to speed.</p>\n"
        "<ul>\n"
        "<li><a class=\"helper\" href=\"/player?x=1&y=1&solution=w\">"
        "White Square</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=1&y=1&solution=b\">"
        "Black Square</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=2&y=1&solution=wb\">"
        "Two Horizontal Squares</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=1&y=2&solution=bw\">"
        "Two Vertical Squares</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=2&y=2&solution=bwwb\">"
        "Checkers</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=3&y=2&solution=bwwbwb\">"
        "3x2</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=2&y=3&solution=bwwbwb\">"
        "2x3</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=3&y=3&solution=wbwbwbwbw\">"
        "Diamond</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=3&y=3&solution=bbbbwbbbw\">"
        "Ring</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=7&y=7&solution=wbbbbbwbwwwwwb"
        "bwbwbwbbwwwwwbbwbbbwbbwwwwwbwbbbbbw\">Smiley (7x7)</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=9&y=11&solution=wwbwbbwwwwwwb"
        "wwbwwwbwbbwbwwwwbbwbbbwwwwbwwwwwwbbbbbbwwwwwwbwwwwwwbbbbbwwwwbbbbbwwww"
        "bbbbbwwwwwbbbwwwww\">Sunflower</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=10&y=10&solution=wbbbbbbbbwbw"
        "wwwwwwwbbwwwwwwwwbbwwbwwbwwbbwwwwwwwwbbwwwwwwwwbbwbwwwwbwbbwwbbbbwwbbw"
        "wwwwwwwbwbbbbbbbbw\">Smiley (10x10)</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=11&y=11&solution=bbbbbbbbbbbb"
        "wwwwwwwwwbbwbbbbbbbwbbwbwwwwwbwbbwbwbbbwbwbbwbwwwbwbwbbwbbbbbwbwbbwwww"
        "wwwbwbbbbbbbbbbwbwwwwwwwwwwbbbbbbbbbbbb\">Spiral</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=11&y=11&board=wwwwwbwwwwwwwww"
        "bwbwwwwwwwbwbwbwwwwwbwbwbwbwwwbwbwbwbwbwbwbwbwbwbwbwbwbwbwbwwwwwbwbwbw"
        "wwwwwwbwbwwwwwwwwwbwwwwbwwwwwwbwwwww\">Pattern 1</a></li>\n"
        "<li><a class=\"helper\" href=\"/player?x=11&y=11&solution=wwwwbbbwwwww"
        "wwbwwwbwwwwwbwbbbwbwwwbwbwwwbwbwbwbwwbwwbwbbwbwbwbwbwbbwbwwbwwbwbwbwbw"
        "wwbwbwwwbwbbbwbwwwwwbwwwbwwwwwwwbbbwwww\">Concentric</a></li>\n"
        "</ul>\n"
        "</div>\n";
    char end[] = "</body>\n</html>\n\n";
    int is_editor;

    stradd(buf, bufend, buflen, start, sizeof(start));

    //parse query string
    char *tofree, *q, *tok, *board = 0, *x = 0, *y = 0, *solution = 0;
    if (query != 0) { //If there is a QUERY_STRING env. variable
        tofree = q = strdup(query);
        while ((tok = strsep(&q, "&"))) {
            if (!strncmp(tok, "x=", 2)) x = strdup(tok + 2);
            if (!strncmp(tok, "y=", 2)) y = strdup(tok + 2);
            if (!strncmp(tok, "board=", 6)) board = strdup(tok + 6);
            if (!strncmp(tok, "solution=", 9)) solution = strdup(tok + 9);
        }
        free(tofree);
    }

    //determine if we are generating the editor or player
    is_editor = !solution;
    if (is_editor)
        stradd(buf, bufend, buflen, picross_editor, sizeof(picross_editor));
    else
        stradd(buf, bufend, buflen, picross_player, sizeof(picross_player));

    //make sure there are no missing fields
    if (!(x && y)) {
        if (is_editor) {
            char sizeform[] = "<form action=\"editor\">"
                "Width (number of squares): "
                "<input type=\"number\" name=\"x\"><br>\n"
                "Height (number of squares): "
                "<input type=\"number\" name=\"y\"><br>\n"
                "<input type=\"submit\" value=\"Submit\">\n"
                "</form>\n</body>\n</html>\n\n";
            stradd(buf, bufend, buflen, sizeform, sizeof(sizeform));
            *final_length = bufend;
            return realloc(buf, bufend + 1);
        } else {
            char size_err[] = "<h3>Error! Width and height must be specified."
                "</h3>\n</body>\n</html>\n\n";
            stradd(buf, bufend, buflen, size_err, sizeof(size_err));
            *final_length = bufend;
            return realloc(buf, bufend + 1);
        }
    }
    
    errno = 0;
    width = strtol(x, &endptr, 10);
    printf("width errno is %i\n", errno);
    if (errno || width < 1 || width > 20) {
        //exit gracefully on error
        if (errno) {
            char width_error[] = "<p>Error: width is inputted incorrectly."
                " Error code is ";
            stradd(buf, bufend, buflen, width_error, sizeof(width_error));
            errcode = snprintf(temp, sizeof(temp), "\"%i\"</p>\n", errno);
            if (errcode < 0 || errcode > sizeof(temp)) {
                char err_error[] = "<p>Error: Error #1 has its own error.</p>";
                stradd(buf, bufend, buflen, err_error, sizeof(err_error));
                printf("error %i for snprintf #1\n", errcode);
            } else {
                stradd(buf, bufend, buflen, temp,
                        strnlen(temp, sizeof(temp)) + 1);
            }
        } else {
            char too_low_error[] = "<p>Error: width is too low or too high. "
                "Please set it to more than 0, but less than 20.</p>\n";
            stradd(buf, bufend, buflen, too_low_error, sizeof(too_low_error));
        }
        stradd(buf, bufend, buflen, end_bit, sizeof(end_bit));
        *final_length = bufend;
        return realloc(buf, bufend + 1);
    }

    errno = 0;
    errno = 0;
    height = strtol(y, &endptr, 10);
    printf("height errno is %i\n", errno);
    if (errno || height < 1 || height > 20) {
        //exit gracefully on error
        if (errno) {
            char height_error[] = "<p>Error: height is inputted incorrectly."
                " Error code is ";
            stradd(buf, bufend, buflen, height_error, sizeof(height_error));
            errcode = snprintf(temp, sizeof(temp), "\"%i\"</p>\n", errno);
            if (errcode < 0 || errcode > sizeof(temp)) {
                char err_error[] = "<p>Error: Error #2 has its own error.</p>";
                stradd(buf, bufend, buflen, err_error, sizeof(err_error));
                printf("error %i for snprintf #2\n", errcode);
            } else {
                stradd(buf, bufend, buflen, temp,
                        strnlen(temp, sizeof(temp)) + 1);
            }
        } else {
            char too_low_error[] = "<p>Error: height is too low or too high. "
                "Please set it to more than 0, but less than 20.</p>\n";
            stradd(buf, bufend, buflen, too_low_error, sizeof(too_low_error));
        }
        stradd(buf, bufend, buflen, end_bit, sizeof(end_bit));
        *final_length = bufend;
        return realloc(buf, bufend + 1);
    }

#ifdef DEBUG
    printf("board is %i\n", board);
#endif
    //fill in rest of board
    int boardsize = width * height;
    board = fillboard(board, boardsize, is_editor ? 'w' : 'u');
    if (!is_editor) {
        solution = fillboard(solution, boardsize, 'w');
    }
#ifdef DEBUG
    printf("board is %i\n", board);
#endif

    makeboard(width, height, board, boardsize, is_editor, solution,
            &buf, &bufend, &buflen);
#ifdef DEBUG
    printf("board is %i\n", board);
#endif

    if (is_editor) {
        int pgsize = sizeof(play_editing_game) + 50 + boardsize;
        char *play_game_tmp = malloc(pgsize); //should be long enough
        errcode = snprintf(play_game_tmp, pgsize, play_editing_game, width,
                height, board);
        if (errcode < 0 || errcode > pgsize) {
            printf("error %i for snprintf #8\n", errcode);
        }
        stradd(buf, bufend, buflen, play_game_tmp, pgsize);
        free(play_game_tmp);
        stradd(buf, bufend, buflen, editor_help, sizeof(editor_help));
    } else {
        stradd(buf, bufend, buflen, player_help, sizeof(player_help));
    }

#ifdef DEBUG
    printf("board is %i\n", board);
#endif
    free(board);
#ifdef DEBUG
    printf("board is %i\n", board);
#endif
    free(x);
    free(y);
    if (solution)
        free(solution);

    stradd(buf, bufend, buflen, end, sizeof(end));
    *final_length = bufend;
    return realloc(buf, bufend + 1);
}
