#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

enum
{
  TOKEN_KIND_T,
  TOKEN_KIND_NT,
};

typedef struct token_t token_t;
typedef struct gen_t gen_t;

struct token_t
{
  int kind;
  char sval[4];
};

struct gen_t
{
  token_t *left;
  const char *sval;
  token_t *right[7];
};

/* non-terminal tokens */
token_t nt_S = { .kind = TOKEN_KIND_NT, .sval = "S" }; /* S */
token_t nt_H = { .kind = TOKEN_KIND_NT, .sval = "H" }; /* H */
token_t nt_M = { .kind = TOKEN_KIND_NT, .sval = "M" }; /* M */
token_t nt_A = { .kind = TOKEN_KIND_NT, .sval = "A" }; /* A */

/* terminal tokens */
token_t t_p = { .kind = TOKEN_KIND_T, .sval = "#" }; /* # */
token_t t_a = { .kind = TOKEN_KIND_T, .sval = "a" }; /* a */
token_t t_d = { .kind = TOKEN_KIND_T, .sval = "d" }; /* d */
token_t t_e = { .kind = TOKEN_KIND_T, .sval = "e" }; /* e */
token_t t_b = { .kind = TOKEN_KIND_T, .sval = "b" }; /* b */

/* empty token */
token_t t_null = { .kind = TOKEN_KIND_T, .sval = "$" };

gen_t gens[] = {
  { .left = &nt_S, .right = { &t_a, &nt_H, &t_p }, .sval = "S->aH#" },

  { .left = &nt_H, .right = { &t_a, &nt_M, &t_d }, .sval = "H->aMd" },
  { .left = &nt_H, .right = { &t_d }, .sval = "H->d" },

  { .left = &nt_M, .right = { &nt_A, &t_b }, .sval = "M->Ab" },
  { .left = &nt_M, .right = { &t_null }, .sval = "M->" },

  { .left = &nt_A, .right = { &t_a, &nt_M }, .sval = "A->aM" },
  { .left = &nt_A, .right = { &t_e }, .sval = "A->e" },
};

int gens_size = sizeof (gens) / sizeof (gens[0]);

bool
nullable (token_t *tok)
{
  if (tok == &t_null)
    return true;

  if (tok->kind == TOKEN_KIND_T)
    return false;

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = &gens[i];
      if (tok != gen->left)
        continue;

      bool can = true;
      for (token_t **ptr = gen->right, *curr; (curr = *ptr); ptr++)
        if (!nullable (curr))
          {
            can = false;
            break;
          }

      if (can)
        return true;
    }

  return false;
}

bool
nullable2 (token_t **toks)
{
  for (token_t **ptr = toks, *curr; (curr = *ptr); ptr++)
    if (!nullable (curr))
      return false;
  return true;
}

void
first_of (token_t *tok, token_t **result)
{
  int num = 0;
  token_t *temp[8];

  if (tok == &t_null)
    goto ret;

  if (tok->kind == TOKEN_KIND_T)
    {
      result[num++] = tok;
      goto ret;
    }

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = &gens[i];

      if (tok != gen->left)
        continue;

      for (token_t **ptr = gen->right, *curr; (curr = *ptr); ptr++)
        {
          first_of (curr, temp);

          for (token_t **ptr = temp, *curr; (curr = *ptr); ptr++)
            result[num++] = curr;

          if (!nullable (curr))
            break;
        }
    }

ret:
  result[num] = NULL;
}

void
first_of2 (token_t **toks, token_t **result)
{
  int num = 0;
  token_t *temp[8];

  for (token_t **ptr = toks, *curr; (curr = *ptr); ptr++)
    {
      if (curr == &t_null)
        break;

      first_of (curr, temp);
      for (token_t **ptr = temp, *curr; (curr = *ptr); ptr++)
        result[num++] = curr;

      if (!nullable (curr))
        break;
    }

ret:
  result[num] = NULL;
}

void
follow_of (token_t *tok, token_t **result)
{
  int num = 0;
  token_t *temp[8];

  if (tok->kind == TOKEN_KIND_T || tok == &nt_S)
    goto ret;

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = &gens[i];

      for (token_t **ptr = gen->right, *curr; (curr = *ptr); ptr++)
        {
          if (tok != curr)
            continue;

          token_t *next;
          if (!(next = *(ptr + 1)))
            break;

          first_of (next, temp);
          for (token_t **ptr = temp, *curr; (curr = *ptr); ptr++)
            result[num++] = curr;

          if (!nullable (next))
            break;
        }
    }

ret:
  result[num] = NULL;
}

gen_t *
gen_of (token_t *left, token_t *tok)
{
  token_t *first[16];
  token_t *follow[8];

  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = &gens[i];
      if (left != gen->left)
        continue;

      first_of2 (gen->right, first);
      for (token_t **ptr = first, *curr; (curr = *ptr); ptr++)
        if (tok == curr)
          return gen;

      if (!nullable2 (gen->right))
        continue;

      follow_of (left, follow);
      for (token_t **ptr = follow, *curr; (curr = *ptr); ptr++)
        if (tok == curr)
          return gen;
    }

  return NULL;
}

/* get next terminal token: NULL is returned if get '\n' */
token_t *
token_next (void)
{
  char ch = getchar ();

  switch (ch)
    {
    case '\n':
      return NULL;
    case '#':
      return &t_p;
    case 'a':
      return &t_a;
    case 'd':
      return &t_d;
    case 'e':
      return &t_e;
    case 'b':
      return &t_b;
    }

  fprintf (stderr, "unknown token %c\n", ch);
  exit (1);
}

int
main (void)
{
  token_t *first[16];
  token_t *follow[8];

  token_t *nts[] = { &nt_S, &nt_H, &nt_M, &nt_A };
  int nts_size = sizeof (nts) / sizeof (nts[0]);

  token_t *ts[] = { &t_p, &t_a, &t_d, &t_e, &t_b };
  int ts_size = sizeof (ts) / sizeof (ts[0]);

  printf ("token\t\tfirst\t\tfollow\n");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%s", nt->sval);

      printf ("\t\t");
      first_of (nt, first);
      for (token_t **ptr = first, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\t\t");
      follow_of (nt, follow);
      for (token_t **ptr = follow, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\n");
    }

  printf ("\n%-16sfirst\n", "gen");
  for (int i = 0; i < gens_size; i++)
    {
      gen_t *gen = &gens[i];
      token_t **toks = gen->right;
      printf ("%-16s", gen->sval);

      first_of2 (toks, first);
      for (token_t **ptr = first, *curr; (curr = *ptr); ptr++)
        printf ("%s ", curr->sval);

      printf ("\n");
    }

  printf ("\n%4s", "");
  for (int i = 0; i < ts_size; i++)
    printf ("%-10s", ts[i]->sval);

  printf ("\n");
  for (int i = 0; i < nts_size; i++)
    {
      token_t *nt = nts[i];
      printf ("%-4s", nt->sval);
      for (int j = 0; j < ts_size; j++)
        {
          token_t *t = ts[j];
          gen_t *gen = gen_of (nt, t);
          printf ("%-10s", gen ? gen->sval : "");
        }
      printf ("\n");
    }
}
