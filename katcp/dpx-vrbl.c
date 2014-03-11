#ifdef KATCP_EXPERIMENTAL

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sysexits.h>

#include <sys/stat.h>

#include <katcp.h>
#include <katpriv.h>
#include <katcl.h>
#include <avltree.h>

#define MAX_DEPTH_VRBL 3

void release_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py);
struct katcp_vrbl_payload *setup_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, unsigned int type);

int add_payload_katcp(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py);

/* string logic *******************************************************************/

int init_string_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  if(py == NULL){
    return -1;
  }

  py->p_type = KATCP_VRT_STRING;
  py->p_union.u_string = NULL;

  return 0;
}

void clear_string_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
#ifdef KATCP_CONSISTENCY_CHECKS
  if(py == NULL){
    fprintf(stderr, "clear string: no variable given\n");
    abort();
  }
  if(py->p_type != KATCP_VRT_STRING){
    fprintf(stderr, "clear string: bad type %u\n", py->p_type);
    abort();
  }
#endif

  if(py->p_union.u_string){
    free(py->p_union.u_string);
    py->p_union.u_string = NULL;
  }

  py->p_type = KATCP_VRT_GONE;

}

int set_string_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py, char *value)
{
  char *ptr;
  int len;
  struct katcp_vrbl_payload *ty;

  if(vx == NULL){
#ifdef KATCP_CONSISTENCY_CHECKS
    abort();
#else
    return -1;
#endif
  }

  if(py == NULL){
    /* WARNING: null assumes the default ... could be tricky if a complicated structure */
    ty = vx->v_payload;
  } else {
    ty = py;
  }

  if(ty->p_type != KATCP_VRT_STRING){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "string variable: bad type %u\n", ty->p_type);
    abort();
#else
    return -1;
#endif
  }

  if(value == NULL){
    if(ty->p_union.u_string){
      free(ty->p_union.u_string);
      ty->p_union.u_string = NULL;
    }
    return 0;
  }

  len = strlen(value) + 1;
  ptr = realloc(ty->p_union.u_string, len);
  if(ptr == NULL){
    return -1;
  }

  memcpy(ptr, value, len);
  ty->p_union.u_string = ptr;
  return 0;
}

int scan_string_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py, char *text, char *path)
{
  /* special case, as scan uses a string as input too, other types not so easy */
  return set_string_vrbl_katcp(d, vx, py, text);
}

int add_string_vrbl_katcp(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  char *ptr;

#ifdef KATCP_CONSISTENCY_CHECKS
  unsigned int valid;
  if(py == NULL){
    fprintf(stderr, "add variable string: no variable given\n");
    abort();
  }
  if(py->p_type != KATCP_VRT_STRING){
    fprintf(stderr, "add variable string: bad type %u\n", py->p_type);
    abort();
  }
  valid = (KATCP_FLAG_FIRST | KATCP_FLAG_LAST);
  if((flags | valid) != valid){
    fprintf(stderr, "add variable string: bad flags 0x%x supplied\n", flags);
    abort();
  }
#endif

  ptr = py->p_union.u_string;

  return add_string_parse_katcl(px, flags | KATCP_FLAG_STRING, ptr);
}

/* tree logic *******************************************************************/

int init_tree_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  if(py == NULL){
    return -1;
  }

  py->p_union.u_tree = create_avltree();

  if(py->p_union.u_tree == NULL){
    py->p_type = KATCP_VRT_GONE;
    return -1;
  }

  py->p_type = KATCP_VRT_TREE;

  return 0;
}

struct clear_tree_variable_state{
  struct katcp_dispatch *s_dispatch;
  struct katcp_vrbl *s_variable;
};

static void callback_clear_tree_vrbl(void *global, char *key, void *payload)
{
  struct clear_tree_variable_state *st;
  struct katcp_vrbl_payload *py;

  py = payload;
  st = global;

  release_vrbl_katcp(st->s_dispatch, st->s_variable, py);
}

void clear_tree_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  struct avl_tree *tree;
  struct clear_tree_variable_state state, *st;

#ifdef KATCP_CONSISTENCY_CHECKS
  if(py == NULL){
    fprintf(stderr, "clear tree: no variable given\n");
    abort();
  }
  if(py->p_type != KATCP_VRT_TREE){
    fprintf(stderr, "clear tree: bad type %u\n", py->p_type);
    abort();
  }
#endif

  tree = py->p_union.u_tree;
  if(tree == NULL){
    return;
  }

  st = &state;

  st->s_dispatch = d;
  st->s_variable = vx;

  destroy_complex_avltree(tree, st, &callback_clear_tree_vrbl);

  py->p_union.u_tree = NULL;
  py->p_type = KATCP_VRT_GONE;
}

#if 0
int set_tree_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py, char *value)
{
  char *ptr;
  int len;
  struct katcp_vrbl_payload *ty;

  if(vx == NULL){
#ifdef KATCP_CONSISTENCY_CHECKS
    abort();
#else
    return -1;
#endif
  }

  if(py == NULL){
    /* WARNING: null assumes the default ... could be tricky if a complicated structure */
    ty = vx->v_payload;
  } else {
    ty = py;
  }

  if(ty->p_type != KATCP_VRT_STRING){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "string variable: bad type %u\n", ty->p_type);
    abort();
#else
    return -1;
#endif
  }

  if(value == NULL){
    if(ty->p_union.u_string){
      free(ty->p_union.u_string);
      ty->p_union.u_string = NULL;
    }
    return 0;
  }

  len = strlen(value) + 1;
  ptr = realloc(ty->p_union.u_string, len);
  if(ptr == NULL){
    return -1;
  }

  memcpy(ptr, value, len);
  ty->p_union.u_string = ptr;
  return 0;
}

int scan_tree_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py, char *text, char *path)
{
  /* special case, as scan uses a string as input too, other types not so easy */
  return set_string_vrbl_katcp(d, vx, py, text);
}
#endif

struct add_tree_variable_state{
  int s_result;
  struct katcl_parse *s_parse;
  struct katcp_vrbl *s_variable;
};

static int callback_add_tree_vrbl(struct katcp_dispatch *d, void *global, char *key, void *data)
{
  int result;
  struct katcp_vrbl_payload *py;
  struct add_tree_variable_state *st;

  /* WARNING: this function disregards _FIRST and _LAST flags, as it is bracketed by { and } */
  /* if that ever changes state could include the flags, however it is unclear how the end of the tree would be detected ... */

  if((key == NULL) || (data == NULL)){
    return -1;
  }

  py = data;
  st = global;

  result = add_payload_katcp(d, st->s_parse, 0, st->s_variable, py);

  if(result < 0){
    st->s_result = (-1);
  } else {
    st->s_result += result;
  }

  return 0;
}

int add_tree_vrbl_katcp(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  struct avl_tree *tree;
  struct add_tree_variable_state state, *st;
  int sum, result;

#ifdef KATCP_CONSISTENCY_CHECKS
  unsigned int valid;

  if(py == NULL){
    fprintf(stderr, "add tree: no variable given\n");
    abort();
  }
  if(py->p_type != KATCP_VRT_TREE){
    fprintf(stderr, "add tree: bad type %u\n", py->p_type);
    abort();
  }
  valid = (KATCP_FLAG_FIRST | KATCP_FLAG_LAST);
  if((flags | valid) != valid){
    fprintf(stderr, "add tree: bad flags 0x%x supplied\n", flags);
    abort();
  }
#endif

  st = &state;

  tree = py->p_union.u_tree;
  sum = 0;

  result = add_string_parse_katcl(px, (flags & KATCP_FLAG_FIRST) | KATCP_FLAG_STRING, "{");
  if(result < 0){
    return -1;
  }
  sum += result;

  st->s_result = 0;
  st->s_parse = px;
  st->s_variable = vx;

  complex_inorder_traverse_avltree(d, tree->t_root, st, &callback_add_tree_vrbl);
  if(st->s_result < 0){
    return -1;
  }
  sum += st->s_result;

  result = add_string_parse_katcl(px, (flags & KATCP_FLAG_LAST) | KATCP_FLAG_STRING, "}");
  if(result < 0){
    return -1;
  }
  sum += result;

  return sum;
}

/* lookup structure for all types *************************************************/

struct katcp_vrbl_type_ops{
  char *t_name;

  int (*t_init)(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py);
  void (*t_clear)(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py);
  int (*t_scan)(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py, char *text, char *path);

  int (*t_add)(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py);
};

struct katcp_vrbl_type_ops ops_type_vrbl[KATCP_MAX_VRT] = {
  [KATCP_VRT_STRING] = 
  { "string",
    &init_string_vrbl_katcp,
    &clear_string_vrbl_katcp,
    &scan_string_vrbl_katcp,
    &add_string_vrbl_katcp
  }
};

/* type name related functions ****************************************************/

int type_from_string_vrbl_katcp(struct katcp_dispatch *d, char *string)
{
  int i;

  if(string == NULL){
    return -1;
  }

  for(i = 0; i < KATCP_MAX_VRT; i++){
    if(!strcmp(string, ops_type_vrbl[i].t_name)){
      return i;
    }
  }

  return -1;
}

char *type_to_string_vrbl_katcp(struct katcp_dispatch *d, unsigned int type)
{
  if(type >= KATCP_MAX_VRT){
    return NULL;
  }

  return ops_type_vrbl[type].t_name;
}

/* WARNING: order important, needs to correspond to bit position */

static char *flag_lookup_vrbl[] = { "environment",  "version", "sensor", NULL };

unsigned int flag_from_string_vrbl_katcp(struct katcp_dispatch *d, char *string)
{
  int i;

  if(string == NULL){
    return 0;
  }

  for(i = 0; flag_lookup_vrbl[i]; i++){
    if(!strcmp(flag_lookup_vrbl[i], string)){
      return 1 << i;
    }
  }

  return 0;
}

char *flag_to_string_vrbl_katcp(struct katcp_dispatch *d, unsigned int flag)
{
  unsigned int i, v;

  if(flag <= 0){
    return NULL;
  }

  v = flag;

  for(i = 0; flag_lookup_vrbl[i]; i++){
    if(v == 1){
      return flag_lookup_vrbl[i];
    }
    v = v >> 1;
  }

  return NULL;
}

/* payload setup/destroy **********************************************************/

struct katcp_vrbl_payload *setup_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, unsigned int type)
{
  struct katcp_vrbl_payload *py;

  if(type >= KATCP_MAX_VRT){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "logic problem while creating a variable: unknown type %u\n", type);
    abort();
#else
    return NULL;
#endif
  }

  py = malloc(sizeof(struct katcp_vrbl_payload));
  if(py == NULL){
    return NULL;
  }

  py->p_type = KATCP_VRT_GONE;

  if((*(ops_type_vrbl[type].t_init))(d, vx, py) < 0){
    free(py);
    return NULL;
  }

  return py;
}

void release_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  if(py == NULL){
    return;
  }

  if(py->p_type == KATCP_VRT_GONE){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "logic failure in variable destruction: variable %p already gone\n", py);
    abort();
#else
    return;
#endif
  }

  if(py->p_type < KATCP_MAX_VRT){
    (*(ops_type_vrbl[py->p_type].t_clear))(d, vx, py);
  }

  py->p_type = KATCP_VRT_GONE;

  free(py);
}

/* generic functions using type ops ***********************************************/

struct katcp_vrbl *create_vrbl_katcp(struct katcp_dispatch *d, unsigned int type, unsigned int flags, int (*refresh)(struct katcp_dispatch *d, char *name, struct katcp_vrbl *vx), int (*change)(struct katcp_dispatch *d, char *name, struct katcp_vrbl *vx), void (*release)(struct katcp_dispatch *d, char *name, struct katcp_vrbl *vx))
{
  struct katcp_vrbl *vx;

  if(type >= KATCP_MAX_VRT){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "logic problem while creating a variable: unknown type %u\n", type);
    abort();
#else
    return NULL;
#endif
  }

  vx = malloc(sizeof(struct katcp_vrbl));
  if(vx == NULL){
    return NULL;
  }

  vx->v_status = 0; /* TODO - nominal ? unknown ? */
  vx->v_flags = flags;

  vx->v_refresh = refresh;
  vx->v_change = change;
  vx->v_release = release;

  vx->v_payload = setup_vrbl_katcp(d, vx, type);
  if(vx->v_payload == NULL){
    free(vx);
    return NULL;
  }

  return vx;
}

void destroy_vrbl_katcp(struct katcp_dispatch *d, char *name, struct katcp_vrbl *vx)
{
  if(vx == NULL){
    return;
  }

  vx->v_change = NULL;
  vx->v_refresh = NULL;

  if(vx->v_release){
    (*(vx->v_release))(d, name, vx);
    vx->v_release = NULL;
  }

  release_vrbl_katcp(d, vx, vx->v_payload);

  vx->v_status = 0;
  vx->v_flags = 0;

  free(vx);
}

int scan_vrbl_katcp(struct katcp_dispatch *d, struct katcp_vrbl *vx, char *text, char *path)
{
  int result;
  struct katcp_vrbl_payload *py;

  if((vx == NULL) || (vx->v_payload == NULL)){
    return -1;
  }

  py = vx->v_payload;

  if(py->p_type < KATCP_MAX_VRT){
    result = (*(ops_type_vrbl[py->p_type].t_scan))(d, vx, py, text, path);
    if(result < 0){
      return -1;
    }
  }

  if(vx->v_change){
    /* WARNING: unclear if this is supposed to happen this far down the API ? */
    /* also unclear if this needs to trigger on an unsuccessful update */
    (*(vx->v_change))(d, NULL, vx);
  }
  
  return result;
}

/************************************************************************************/

int add_payload_katcp(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx, struct katcp_vrbl_payload *py)
{
  if(vx == NULL){
    return -1;
  }
  
  if((py->p_type >= KATCP_MAX_VRT) || (py->p_type < 0)){
#ifdef KATCP_CONSISTENCY_CHECKS
    fprintf(stderr, "logic failure: invalid type %u while outputting variable %p\n", py->p_type, vx);
    abort();
#else
    return -1;
#endif
  }

  return (*(ops_type_vrbl[py->p_type].t_add))(d, px, flags, vx, py);
}

int add_vrbl_katcp(struct katcp_dispatch *d, struct katcl_parse *px, int flags, struct katcp_vrbl *vx)
{
  if(vx == NULL){
    return -1;
  }

  return add_payload_katcp(d, px, flags, vx, vx->v_payload);
}

/* variable region functions ******************************************************/

struct katcp_vrbl *find_region_katcp(struct katcp_dispatch *d, struct katcp_region *rx, char *key)
{
  if(rx == NULL){
    return NULL;
  }

  return find_data_avltree(rx->r_tree, key);
}

int insert_region_katcp(struct katcp_dispatch *d, struct katcp_region *rx, struct katcp_vrbl *vx, char *key)
{
  if(rx == NULL){
    return -1;
  }

  if(store_named_node_avltree(rx->r_tree, key, vx) < 0){
    return -1;
  }

  return 0;
}

/* full variable layer functions **************************************************/

struct katcp_vrbl *find_vrbl_katcp(struct katcp_dispatch *d, char *key)
{
  struct katcp_region *vra[MAX_DEPTH_VRBL];
  struct katcp_flat *fx;
  struct katcp_group *gx;
  struct katcp_shared *s;
  struct katcp_vrbl *vx;
  unsigned int i;

  fx = this_flat_katcp(d);
  gx = this_group_katcp(d);
  s = d->d_shared;

  if((fx == NULL) || (gx == NULL) || (s == NULL)){
    return NULL;
  }

  vra[0] = fx->f_region;
  vra[1] = gx->g_region;
  vra[2] = s->s_region;

  for(i = 0; i < MAX_DEPTH_VRBL; i++){
    vx = find_region_katcp(d, vra[i], key);
    if(vx){
      return vx;
    }
  }

  return NULL;
}

int traverse_vrbl_katcp(struct katcp_dispatch *d, void *state, int (*callback)(struct katcp_dispatch *d, void *state, char *key, void *data))
{
  struct katcp_region *vra[MAX_DEPTH_VRBL];
  struct katcp_flat *fx;
  struct katcp_group *gx;
  struct katcp_shared *s;
  unsigned int i;

  fx = this_flat_katcp(d);
  gx = this_group_katcp(d);
  s = d->d_shared;

  if((fx == NULL) || (gx == NULL) || (s == NULL)){
#ifdef DEBUG
    fprintf(stderr, "traverse: insufficient context available\n");
#endif
    return -1;
  }

  vra[0] = fx->f_region;
  vra[1] = gx->g_region;
  vra[2] = s->s_region;

  for(i = 0; i < MAX_DEPTH_VRBL; i++){
#ifdef DEBUG
    fprintf(stderr, "traverse: checking[%d]=%p\n", i, vra[i]);
#endif
    complex_inorder_traverse_avltree(d, vra[i]->r_tree->t_root, state, callback);
  }

  return 0;
}

/* traverse the hierachy of variables regions ****************/

/* 
 * exact absolute
 * > var@root*                    (1)
 * > group*var@group*             (2)
 * > group*flat*var@flat*         (3)
 *
 * exact relative
 * > *var@curgrouponly*           (2)
 * > **var@curflatonly*           (3)
 *
 * search
 * > varinflatthengroupthenroot   (0)
 * > *varinflatthengroup          (1)
 */

struct katcp_vrbl *update_vrbl_katcp(struct katcp_dispatch *d, struct katcp_flat *fx, char *name, struct katcp_vrbl *vo, int clobber)
{
#define MAX_STAR 3
  struct katcp_shared *s;
  struct katcp_group *gx;
  struct katcp_flat *fy;
  struct katcp_vrbl *vx;
  struct katcp_region *vra[MAX_DEPTH_VRBL];
  int i, count, last;
  char *copy, *var;
  unsigned int v[MAX_STAR];

  copy = strdup(name);
  if(copy == NULL){
    return NULL;
  }

  s = d->d_shared;

  count = 0;
  for(i = 0; copy[i] != '\0'; i++){
    if(copy[i] == '*'){
      if(count >= MAX_STAR){
        free(copy);
        return NULL;
      }
      v[count] = i;
      copy[i] = '\0';
      count++;
    }
  }
  if(i <= 0){
    free(copy);
    return NULL;
  }

  last = i - 1;

#ifdef KATCP_CONSISTENCY_CHECKS
  if(fx){
    if(fx->f_group == NULL){
      fprintf(stderr, "major logic problem: duplex instance %p (%s) not a member of any group\n", fx, fx->f_name);
      abort();
    }
  }
  if(s == NULL){
    fprintf(stderr, "major logic problem: no global state available\n");
    abort();
  }
#endif

  vra[0] = NULL;
  vra[1] = NULL;
  vra[2] = NULL;

  var = NULL;

  switch(count){
    case 0 : /* search in current flat, then current group, then root */
      if(fx == NULL){
        free(copy);
        return NULL;
      }
      var = copy;
      vra[0] = fx->f_region;
      vra[1] = fx->f_group->g_region;
      vra[2] = s->s_region;
      break;
    case 1 :
      if(v[0] == 0){ /* search in current flat, then current group */
        if(fx == NULL){
          free(copy);
          return NULL;
        }
        var = copy + 1;
        vra[0] = fx->f_region;
        vra[1] = fx->f_group->g_region;
      } else if(v[0] == last){ /* search in root */
        var = copy;
        vra[0] = s->s_region;
      } else {
        free(copy);
        return NULL;
      }
      break;
    case 2 : 
      if(v[1] != last){
        free(copy);
        return NULL;
      }
      if(v[0] == 0){ /* search in current group */
        if(fx == NULL){
          free(copy);
          return NULL;
        }
        var = copy + 1;
        vra[0] = fx->f_group->g_region;
      } else { /* search in named group */
        gx = find_group_katcp(d, copy);
        if(gx == NULL){
          free(copy);
          return NULL;
        }
        vra[0] = gx->g_region;
        var = copy + v[1] + 1;
      }
      break;
    case 3 : 
      if(v[0] == 0){ /* search in current flat */
        if(v[1] != 1){
          free(copy);
          return NULL;
        }
        vra[0] = fx->f_region;
        var = copy + 2;
      } else { /* search in named flat */

        fy = find_name_flat_katcp(d, copy + 1, copy + v[1] + 1);
        if(fy == NULL){
          free(copy);
          return NULL;
        }

        vra[0] = fy->f_region;
        var = copy + v[2] + 1;
      }
      break;
    default :
      free(copy);
      return NULL;
  }

  if((var == NULL) || (var[0] == '\0')){
    free(copy);
    return NULL;
  }

#ifdef KATCP_CONSISTENCY_CHECKS
  if(vra[0] == NULL){
    fprintf(stderr, "major logic problem: attempting to update variable without any target location\n");
    abort();
  }
#endif

  vx = NULL;
  i = 0;

  do{
    vx = find_region_katcp(d, vra[i], var);
    i++;
  } while((i < MAX_DEPTH_VRBL) && (vra[i] != NULL) && (vx == NULL));

  if(vx == NULL){
    if(vo == NULL){
      free(copy);
      return NULL;
    }

    if(insert_region_katcp(d, vra[0], vo, var) < 0){
      free(copy);
      return NULL;
    }

    free(copy);
    return vo;

  } else {

    if(clobber){
      free(copy);
      return NULL;
    }

    if(vo){
      /* TODO: replace content of variable */
#if 1
      fprintf(stderr, "usage problem: no insitu update implemented yet\n");
      abort();
#endif
    }

    return vx;
  }

#undef MAX_STAR
}

/* setup logic ***********************************************/

void destroy_vrbl_for_region_katcp(void *global, char *key, void *payload)
{
  struct katcp_dispatch *d;
  struct katcp_vrbl *vx;

  d = global;
  vx = payload;

  destroy_vrbl_katcp(d, key, vx);
}

void destroy_region_katcp(struct katcp_dispatch *d, struct katcp_region *rx)
{
  if(rx == NULL){
    return;
  }

  if(rx->r_tree){
    destroy_complex_avltree(rx->r_tree, d, &destroy_vrbl_for_region_katcp);
    rx->r_tree = NULL;
  }

  free(rx);
}

struct katcp_region *create_region_katcp(struct katcp_dispatch *d)
{
  struct katcp_region *rx;

  rx = malloc(sizeof(struct katcp_region));
  if(rx == NULL){
    return NULL;
  }

  rx->r_tree = create_avltree();

  if(rx->r_tree == NULL){
    destroy_region_katcp(d, rx);
    return NULL;
  }

  return rx;
}

/* top-level string API functions *********************************************/

struct katcp_vrbl *create_string_vrbl_katcp(struct katcp_dispatch *d, unsigned int options, char *value)
{
  struct katcp_vrbl *vx;

  vx = create_vrbl_katcp(d, KATCP_VRT_STRING, options, NULL, NULL, NULL);
  if(vx == NULL){
    return NULL;
  }

  if(set_string_vrbl_katcp(d, vx, NULL, value) < 0){
    destroy_vrbl_katcp(d, NULL, vx);
    return NULL;
  }

  return vx;
}

int make_string_vrbl_katcp(struct katcp_dispatch *d, struct katcp_group *gx, char *key, unsigned int options, char *value)
{
  struct katcp_region *rx;
  struct katcp_shared *s;
  struct katcp_vrbl *vx;

  s = d->d_shared;

  if(gx){
    rx = gx->g_region;
  } else {
    rx = s->s_region;
  }

  if(rx == NULL){
    return -1;
  }

  if(find_region_katcp(d, rx, key)){
    return -1;
  }

  vx = create_string_vrbl_katcp(d, options, value);
  if(vx == NULL){
    return -1;
  }

  if(insert_region_katcp(d, rx, vx, key) < 0){
    destroy_vrbl_katcp(d, key, vx);
    return -1;
  }

  return 0;
} 

/****/

#if 0

int insert_string_region_katcp(struct katcp_dispatch *d, struct katcp_region *rx, char *key, unsigned int flags, char *value)
{
  struct katcp_vrbl *vx;

  vx = create_string_vrbl_katcp(d, flags, value);
  if(vx == NULL){
    return -1;
  }

  if(insert_region_katcp(d, rx, vx, key) < 0){
    destroy_vrbl_katcp(d, key, vx);
    return -1;
  }

  return 0;
}
#endif

/* commands operating on variables ********************************************/

int var_declare_group_cmd_katcp(struct katcp_dispatch *d, int argc)
{
  char *name, *copy, *ptr, *current;
  int type, result;
  unsigned int options, flag;
  struct katcp_vrbl *vx;
  struct katcp_flat *fx;

  if(argc < 2){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "need a name");
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
  }

  name = arg_string_katcp(d, 1);
  if(name == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "unable to process a null name");
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
  }

  options = 0;
  type = (-1);

  if(argc > 2){
    copy = arg_copy_string_katcp(d, 2);
    if(copy == NULL){
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "type field may not be null");
      return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
    }

    current = copy;

    do{
      ptr = strchr(current, ',');
      if(ptr){
        ptr[0] = '\0';
      }

      flag = flag_from_string_vrbl_katcp(d, current);
      if(flag){
        options |= flag;
      } else {
        result = type_from_string_vrbl_katcp(d, current);

        if(result < 0){
          log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "unknown variable attribute %s for %s", current, name);
          free(copy);
          return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
        }

        if((type >= 0) && (type != result)){
          log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "conflicting type attribute %s for %s", current, name);
          free(copy);
          return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
        }

        type = result;
      }

      if(ptr){
        current = ptr + 1;
      }

    } while(ptr != NULL);

    free(copy);
  }

  if(type < 0){
    type = KATCP_VRT_STRING;
  }

  fx = this_flat_katcp(d);
  if(fx == NULL){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_API);
  }

  vx = create_vrbl_katcp(d, type, options, NULL, NULL, NULL);
  if(vx == NULL){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_MALLOC);
  }

  if(update_vrbl_katcp(d, fx, name, vx, 0) == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "unable to declare new variable %s", name);
    destroy_vrbl_katcp(d, name, vx);
    return KATCP_RESULT_FAIL;
  }

  log_message_katcp(d, KATCP_LEVEL_DEBUG, NULL, "created variable %s with type %d and flags 0x%x", name, type, options);

  return KATCP_RESULT_OK;
}

/* variable listing - callback then command ***************************/

int var_list_callback_katcp(struct katcp_dispatch *d, void *state, char *key, struct katcp_vrbl *vx)
{
#define BUFFER 128
  char *ptr, *tmp;
  unsigned int i;
  unsigned int used, have, len;
  struct katcp_vrbl_payload *py;

#ifdef KATCP_CONSISTENCY_CHECKS
  if((vx == NULL) || (vx->v_payload == NULL)){
    fprintf(stderr, "variables: major logic problems: null data structure passed to listing function\n");
    abort();
  }
#endif

  py = vx->v_payload;

  ptr = malloc(BUFFER);
  if(ptr == NULL){
    return -1;
  }
  have = BUFFER;

  tmp = type_to_string_vrbl_katcp(d, py->p_type);
  if(tmp == NULL){
    free(ptr);
    return -1;
  }

  len = strlen(tmp);

  memcpy(ptr, tmp, len + 1);
  used = len;

  for(i = 0; flag_lookup_vrbl[i]; i++){
    if((1 << i) & vx->v_flags){
#ifdef DEBUG
      fprintf(stderr, "vrbl: flag match[%u]=%s\n", i, flag_lookup_vrbl[i]);
#endif
      len = strlen(flag_lookup_vrbl[i]);
      if((used + 1 + len + 1) > have){
        have = used + 1 + len + 1;
        tmp = realloc(ptr, have);
        if(tmp == NULL){
          free(ptr);
          return -1;
        }
        ptr = tmp;
      }
      ptr[used] = ',';
      used++;
      memcpy(ptr + used, flag_lookup_vrbl[i], len + 1);
      used += len;
    }
  }

  prepend_inform_katcp(d);
  append_string_katcp(d, KATCP_FLAG_STRING, key);
  append_string_katcp(d, KATCP_FLAG_STRING, ptr);
  append_vrbl_katcp(d, KATCP_FLAG_LAST, vx);

  free(ptr);

  return 0;
#undef BUFFER
}

int var_list_void_callback_katcp(struct katcp_dispatch *d, void *state, char *key, void *data)
{
  struct katcp_vrbl *vx;

  vx = data;

  return var_list_callback_katcp(d, state, key, vx);
}

int var_list_group_cmd_katcp(struct katcp_dispatch *d, int argc)
{
  char *key;
  unsigned int i;
  int result;
  struct katcp_flat *fx;
  struct katcp_group *gx;
  struct katcp_shared *s;
  struct katcp_vrbl *vx;

  fx = this_flat_katcp(d);
  gx = this_group_katcp(d);
  s = d->d_shared;

  if((fx == NULL) || (gx == NULL) || (s == NULL)){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_API);
  }

  result = 0;

  if(argc > 1){
    for(i = 1 ; i < argc ; i++){
      key = arg_string_katcp(d, i);
      if(key == NULL){
        return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
      }
      vx = find_vrbl_katcp(d, key);
      if(vx == NULL){
        log_message_katcp(d, KATCP_LEVEL_WARN, NULL, "%s not found", key);
        result = (-1);
      } else {
        if(var_list_callback_katcp(d, NULL, key, vx) < 0){
          result = (-1);
        }
      }
    }
  } else {
    result = traverse_vrbl_katcp(d, NULL, &var_list_void_callback_katcp);
  }

  if(result < 0){
    return KATCP_RESULT_FAIL;
  }

  return KATCP_RESULT_OK;
}

/* set a previously declared variable ********************************/

int var_set_group_cmd_katcp(struct katcp_dispatch *d, int argc)
{
  char *key, *value;
  struct katcp_vrbl *vx;

  if(argc <= 2){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
  }

  key = arg_string_katcp(d, 1);
  if(key == NULL){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
  }

  vx = find_vrbl_katcp(d, key);
  if(vx == NULL){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_NOT_FOUND);
  }

  value = arg_string_katcp(d, 2);
  if(value == NULL){
    return extra_response_katcp(d, KATCP_RESULT_FAIL, KATCP_FAIL_USAGE);
  }

  /* eh, how does one set paths ? */
  if(scan_vrbl_katcp(d, vx, value, NULL) < 0){
    return KATCP_RESULT_FAIL;
  }

  return KATCP_RESULT_OK;
}

#endif