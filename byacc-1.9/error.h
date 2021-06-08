#ifndef ERROR_H_INCLUDED
#define ERROR_H_INCLUDED

extern void fatal(char *msg);
extern void no_space(void);
extern void open_error(char *filename);

extern void unterminated_comment(int c_lineno, char *c_line, char *c_cptr);
extern void syntax_error(int st_lineno, char *st_line, char *st_cptr);
extern void unexpected_EOF(void);
extern void unterminated_text(int t_lineno, char *t_line, char *t_cptr);
extern void unterminated_string(int s_lineno, char *s_line, char *s_cptr);
extern void over_unionized(char *u_cptr);
extern void unterminated_union(int u_lineno, char *u_line, char *u_cptr);
extern void illegal_character(char *c_cptr);
extern void used_reserved(char *s);
extern void illegal_tag(int t_lineno, char *t_line, char *t_cptr);
extern void tokenized_start(char *s);
extern void retyped_warning(char *s);
extern void reprec_warning(char *s);
extern void revalued_warning(char *s);
extern void terminal_start(char *s);
extern void restarted_warning(void);
extern void no_grammar(void);
extern void terminal_lhs(int s_lineno);
extern void default_action_warning(void);
extern void dollar_warning(int a_lineno, int i);
extern void dollar_error(int a_lineno, char *a_line, char *a_cptr);
extern void untyped_lhs(void);
extern void unknown_rhs(int i);
extern void untyped_rhs(int i, char *s);
extern void unterminated_action(int a_lineno, char *a_line, char *a_cptr);
extern void prec_redeclared(void);
extern void undefined_goal(char *s);
extern void undefined_symbol_warning(char *s);

#endif
