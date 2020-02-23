int single_int_on_line_1_col_4;
int  single_int_on_line_2_col_5;

single_int_on_line_1_col_4 = 3;
 single_int_on_line_2_col_5 = 5;

void function(a, b) {}

function(a?b:c,);               /* Attempting to trigger
                                   COMPL_UP_FUN_PROFILE rule but it
                                   seems that it is not used or
                                   triggered */
