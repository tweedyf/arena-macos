/*

howcome  27/9/95:

This is the style sheet module (i.e. the "styler") of Arena. It will
parse CSS style sheets into structures that can be queried by the
formatter of e.g an HTML browser.

Typically, the application will create a new style sheet initialized
with the application fallback values. For this, use StyleGetInit

Then, if the user has a personal style sheet -- or if incoming
documents have style sheets -- they can be parsed and added to an
exsiting style sheet by StyleChew

As the HTML/SGML parser encounters tags, FormatElementStart should be
called to have style properties evaluated for the specific element in
this specific context. The set of properties are pushed onto a stack
which corresponds to the document structure.

The formatter can query the style properties of the current element
StyleGet.

FormatElementEnd pops a set of style properties from the stack and
shoul be called when the parser reports the end of an element.


History:

howcome 15/2/94: written from scratch

Dave Raggett:

How about the following notation for style:

        * -> font.name = times

        H1 -> font.name = garamond
        H[2-6] -> font.name = helvetica
        
        P.CLASS = abstract ->
            {
                font.name = helvetica
                font.size = 10 pt
                indent.left = 4 em
                indent.right = 4 em
            }

The left hand side of the "->" operator matches tag names and uses
the dot notation for attribute values. You can use regular expressions
such as [digit-digit] [letter-letter] * and ?. The browser is responsible
for sorting the rules in order of most to least specificity so it doesn't
matter that the first rule matches anything.

The right hand side simply sets property values. The HTML formatter knows
what properties are relevant to which elements. Some properties are
specific to one kind of element, while others are generic. Properties
set by a more specific rule (matching the current element) override values
set by least specific rules or values inherited from nested or peer elements.


howcome 1/4/95: 

  Dave's proposal for syntax has been implemented with some shortcuts.
  The "->" construct was replaced by ":" whish should have the same associations. 

  The "P.CLASS = abstract" construct is more general than required for
  the class attribute. As a shortcut, "P [abstract]" has been implemented.

howcome 5/4/95: 

  pattern searches implemented /H1 P/: color.text = #FFF

  Also, commands can be put on the same line with a ";" separating.

  # this is a comment
  / so is this

howcome 5/95: changes to the syntax:

  class is now always denoted p.foo
  elemets and hints can be grouped:

  H1, H2: font.family = helvetica; font.color = #F00

howcome 8/95:

  style sheet module updated to support the august 10 version of

    http://www.w3.org/hypertext/WWW/Style/css/draft.html

*/


#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "www.h"

extern int debug;
extern Doc *CurrentDoc;
extern Context *context;
extern int PixOffset;
extern int paintlen;
extern int paintStartLine;
extern Byte *paint;
extern unsigned int ui_n;
extern unsigned int win_width, win_height;
extern int fontsize;
extern int prepass;
extern int sbar_width;
extern unsigned int win_width, win_height;
extern int ToolBarHeight;
extern int statusHeight;
extern int damn_table;

double lens_factor = 1.0;

extern int math_normal_sym_font, math_small_sym_font,
    math_normal_text_font, math_small_text_font, math_italic_text_font,
    math_bold_text_font;





HTList *style_stack = NULL;
StyleFlat *current_flat;
StyleSheet *current_sheet;

/* 
  str2list is a convenience function to make a list out of two strings
*/

HTList *str2list(char *s, char *t)
{
    HTList *l = HTList_new();

    if (t)
	HTList_addObject(l, strdup(t));
    if (s)
	HTList_addObject(l, strdup(s));
    return l;			      
}

/*
  pt2px converts points to pixels

  display_pt2px =  (25.4 / 72.0) * 
    (((double) DisplayWidth(display,screen)) / ((double) DisplayWidthMM(display, screen)));
*/


extern double display_pt2px;

int pt2px(double pt)
{
    return (int) ((display_pt2px * pt) + 0.5);
}


/* 
  The Flag functions are used to remember various settings. E.g., if
  there is a big initial, we want that property to inherit to child
  elements, but we do not want it to be used more than once. Depending
  on your HTML/SGML parser, this problem may have more elegant solution
*/


BOOL StyleGetFlag(int flag)
{
    switch (flag)
    {
    case S_INITIAL_FLAG:
	return current_sheet->initial_flag;
	break;
    case S_LEADING_FLAG:
	return current_sheet->leading_flag;
	break;
    case S_INDENT_FLAG:
	return current_sheet->indent_flag;
	break;
    case S_MARGIN_TOP_FLAG:
	return current_sheet->margin_top_flag;
	break;
    }
    return FALSE;
}

void StyleSetFlag(int flag, BOOL value)
{
    switch (flag)
    {
    case S_INITIAL_FLAG:
	current_sheet->initial_flag = value;
	break;
    case S_LEADING_FLAG:
	current_sheet->leading_flag = value;
	break;
    case S_INDENT_FLAG:
	current_sheet->indent_flag = value;
	break;
    case S_MARGIN_TOP_FLAG:
	current_sheet->margin_top_flag = value;
	break;	
    }
}




/* methods */


StyleRule *NewStyleRule()
{
    return (StyleRule *)calloc(1,sizeof(StyleRule));
}


StyleSelector *NewStyleSelector()
{
    return (StyleSelector *)calloc(1,sizeof(StyleSelector));
}

StyleSheet *NewStyleSheet()
{
    return (StyleSheet *)calloc(1,sizeof(StyleSheet));
}

/* NewStyleFlat allocates memory AND sets initial values. CHANGE! */

StyleFlat *NewStyleFlat()
{
    StyleFlat *sf;

    sf = (StyleFlat *)calloc(1, sizeof(StyleFlat));

    sf->value[S_MARGIN_LEFT] = (void *)20; /* 30 */
    sf->status[S_MARGIN_LEFT] = S_INITIALIZED;
    sf->value[S_MARGIN_RIGHT] = (void *)20; /* 20 */
    sf->status[S_MARGIN_RIGHT] = S_INITIALIZED;
    sf->value[S_MARGIN_TOP] = (void *)5; /* 5 */
    sf->status[S_MARGIN_TOP] = S_INITIALIZED;
    sf->value[S_MARGIN_BOTTOM] = (void *)5; /* 5 */
    sf->status[S_MARGIN_BOTTOM] = S_INITIALIZED;
    return sf;
}


void FreeStyleSelector(StyleSelector *selector)
{
    StyleSelector *prev_selector;

    prev_selector = selector;

    while((selector = selector->ancestor)) {
	Free(prev_selector->unit.class);
	Free(prev_selector);
	prev_selector = selector;
    }
    if(!prev_selector)
    {
	Free(prev_selector->unit.class);
	Free(prev_selector);
    }
}
	

void FreeStyleRule(StyleRule *r)
{
    if (!r)
	return;

    FreeStyleSelector(r->selector);
    Free(r->warning);
}
    

void FreeStyleSheet(StyleSheet *s)
{
    HTList *l;
    StyleRule *r;
    int i;

    if (!s)
	return;

    for(i = 0; i<TAG_LAST; i++) {
	if ((l = s->element[i]))
	    while ((r = (StyleRule *)HTList_nextObject(l)))
		FreeStyleRule(r);
    }
    Free(s);
}

/*
void StyleFree(StyleSheet *sheet)
{
    int i;

    for(i=0; i<TAG_LAST; i++) {
	HTList_destroy(sheet->element[i]);
    }
    Free(sheet);
}
*/

StyleSelector *CopySelector(StyleSelector *s)
{
    StyleSelector *sc;

    if (!s)
	return NULL;

    sc = NewStyleSelector();
    memcpy(sc, s, sizeof(StyleSelector));
    sc->ancestor = CopySelector (s->ancestor);
    return(sc);
}

StyleSheet *StyleCopy(StyleSheet *s)
{
    StyleSheet *sc = NewStyleSheet();
    int i;
    HTList *l, *lc;
    StyleRule *r, *rc;

    for (i=0; i<TAG_LAST; i++) {
	l = s->element[i];
	lc = sc->element[i];
	while ( (r = (StyleRule *) HTList_nextObject(l) )) {	
	    rc = NewStyleRule();
	    rc->property = r->property;
	    rc->value = r->value;
	    rc->weight = r->weight;
	    if (r->warning)
		rc->warning = strdup(r->warning);

	    rc->selector = CopySelector(r->selector);
	    if (!lc)
		lc = sc->element[i] = HTList_new();
	    HTList_addObject(lc, (void *) rc);
	}
    }
    return sc;
}


/* find, and grade selector matches */

/* 
   SelectorUnitMatch matches two "selector utits", i.e. an element
   with accompanying class (and, in level 2, attributes in general.

   r = rule, s = stack

   The gradins schems is simple: 

   0 = failure, no match
   1 = element matches, no classes specified in rule
   2 = element matches, classes specified and matches

*/

int SelectorUnitMatch(StyleSelectorUnit *r, StyleSelectorUnit *s)
{
    if (r->element == s->element) {
	if (r->class) {
	    if ((s->class) && (strcasecmp(r->class, s->class) == 0)) {
		return 2;
	    }
	    return 0;
	}
	return 1;
    }
    return 0;
}		

int SelectorMatch(StyleSelector *s, StyleSelectorUnit *unit_p, HTList *style_stack)
{
    HTList *l = style_stack;
    StyleStackElement *stack_el;
    int score = 0;

    if (! (score = SelectorUnitMatch(&s->unit, unit_p)))
	return score;

    s = s->ancestor;
    if (!s)
	return score;

    while (s && (stack_el = (StyleStackElement *)HTList_nextObject(l))) {
	if ( (score += SelectorUnitMatch(&s->unit, &stack_el->unit)) )
	    s = s->ancestor;
	if (!s)
	    return 0;
    }
    return score;
}


/* 
   StyleEval flattens the whole list of properties. One could argue
   that properties should only be evaluated when needed. That would be
   a little more complicated.
*/


StyleFlat *StyleEval(StyleSheet *sheet, StyleSelectorUnit *unit_p, HTList *style_stack)
{
    StyleStackElement *stack_el;
    StyleFlat *last_flat = NULL, *flat = NewStyleFlat();
    StyleRule *rule;
    StyleProperty i;
    HTList *l;
    BOOL recompute_font = FALSE, recompute_alt_font = FALSE;
    long value;
    int selector_score;

    if (REGISTER_TRACE)
	fprintf(stderr,"StyleEval: %d, depth of list: %d\n", unit_p->element, HTList_count(style_stack));

    /* init flat */

    stack_el = (StyleStackElement *)HTList_lastObject(style_stack);

    if (stack_el) {
	last_flat = stack_el->flat;
	
	/* copy the inherited properties from ancestor */
	
	for (i=S_UNKNOWN; i < S_NPROPERTY; i++) {
	    switch (i) {

		/* these properies are not inherited */
		
	      case S_MARGIN_TOP:
	      case S_MARGIN_RIGHT:
	      case S_MARGIN_BOTTOM:
	      case S_MARGIN_LEFT:
	      case S_INDENT:
	      case S_PADDING:
	      case S_BACKGROUND:
	      case S_ALT_BACKGROUND:
		break;
		
	      default:
		if ((last_flat->status[i] == S_INHERITED) || (last_flat->status[i] == S_INFERRED)) {
		    flat->value[i] = last_flat->value[i];
		    flat->weight[i] = last_flat->weight[i];
		    flat->status[i] = S_INHERITED;
		}
	    }
	}
    }

    /* go through list of rules */

    l = sheet->element[unit_p->element];

    while ( (rule = (StyleRule *) HTList_nextObject(l) )) {
	if ((selector_score = SelectorMatch(rule->selector, unit_p, style_stack)) && (rule->weight + selector_score >= flat->weight[rule->property])) {
	    flat->value[rule->property] = rule->value;
	    flat->weight[rule->property] = rule->weight + selector_score;
	    flat->status[rule->property] = S_INFERRED;

	    switch (rule->property) {
	      case S_FONT_SHORTHAND:
	      case S_FONT_FAMILY:
	      case S_FONT_SIZE:
/*	      case S_FONT_SIZE_INDEX:*/
	      case S_FONT_WEIGHT:
	      case S_FONT_STYLE:
/* 	      case S_FONT_LEADING: */
		recompute_font = TRUE;
		break;

	      case S_ALT_FONT_SHORTHAND:
	      case S_ALT_FONT_FAMILY:
	      case S_ALT_FONT_SIZE:
/*	      case S_ALT_FONT_SIZE_INDEX:*/
	      case S_ALT_FONT_WEIGHT:
	      case S_ALT_FONT_STYLE:
/* 	      case S_ALT_FONT_LEADING: */
		recompute_alt_font = TRUE;
		break;

	      case S_TEXT_EFFECT:
		if ((int)flat->value[rule->property] == TEXT_EFFECT_INITIAL_CAP)
		    StyleSetFlag(S_INITIAL_FLAG, TRUE);

	      default:
		break;
	    }
	}
    }

    /* what are the compound properties? i.e. properties that depend
       on other properties? is font the only one? */ 

    if (recompute_font) {
	flat->value[S_FONT] = (void *)GetFont((HTList *)flat->value[S_FONT_FAMILY], (int)flat->value[S_FONT_SIZE], 
				      (int)flat->value[S_FONT_WEIGHT], (int)flat->value[S_FONT_STYLE], False);
	flat->status[S_FONT] = S_INFERRED;
    }

    if (recompute_alt_font) {
	flat->value[S_ALT_FONT] = (void *)
	    GetFont(
		    (HTList *)((flat->status[S_ALT_FONT_FAMILY] == S_INFERRED) ? 
			       (int)flat->value[S_ALT_FONT_FAMILY] : (int)flat->value[S_FONT_FAMILY]),
		    ((flat->status[S_ALT_FONT_SIZE] == S_INFERRED) ? 
		     (int)flat->value[S_ALT_FONT_SIZE] : (int)flat->value[S_FONT_SIZE]),
		    ((flat->status[S_ALT_FONT_WEIGHT] == S_INFERRED) ? 
		     (int)flat->value[S_ALT_FONT_WEIGHT] : (int)flat->value[S_FONT_WEIGHT]),
		    ((flat->status[S_ALT_FONT_STYLE] == S_INFERRED) ? 
		     (int)flat->value[S_ALT_FONT_STYLE] : (int)flat->value[S_FONT_STYLE]),
		    False);
	flat->status[S_ALT_FONT] = S_INFERRED;
    }


    /* we cannot compute the colormap index until we know if the color is blinking or not */

    if (flat->status[S_COLOR] == S_INFERRED) {

	if (flat->status[S_ALT_COLOR] != S_INFERRED) {
	    flat->status[S_ALT_COLOR] = S_INFERRED;
	    flat->value[S_ALT_COLOR] = flat->value[S_COLOR];
	    flat->weight[S_ALT_COLOR] = flat->weight[S_COLOR];
	}

	value = (int)flat->value[S_COLOR];
	flat->value[S_COLOR] = (void *)rgb2ix( (int)(value >> 24), (int)((value >> 16) & 0xFF), 
					    (int)((value >> 8) & 0xFF), (int)(value & 0xFF), False);
	if (REGISTER_TRACE && VERBOSE_TRACE)
	    fprintf(stderr,"--> TEXT_COLOR = %ld\n",(int)flat->value[S_COLOR]);
    }

    if (flat->status[S_BACKGROUND] == S_INFERRED) {

	if (flat->status[S_ALT_BACKGROUND] != S_INFERRED) {
	    flat->status[S_ALT_BACKGROUND] = S_INFERRED;
	    flat->value[S_ALT_BACKGROUND] = flat->value[S_BACKGROUND];
	    flat->weight[S_ALT_BACKGROUND] = flat->weight[S_BACKGROUND];
	}
/*
	value = flat->value[S_BACKGROUND];
	flat->value[S_BACKGROUND] = rgb2ix( (int)(value >> 24), (int)((value >> 16) & 0xFF), 
						 (int)((value >> 8) & 0xFF), (int)(value & 0xFF), False);
	if (REGISTER_TRACE && VERBOSE_TRACE)
	    fprintf(stderr,"-> S_BACKGROUND = %ld\n",flat->value[S_BACKGROUND]);
*/
    }


    if (flat->status[S_ALT_COLOR] == S_INFERRED) {
	value = (int)flat->value[S_ALT_COLOR];
	flat->value[S_ALT_COLOR] = (void *)rgb2ix( (int)(value >> 24), (int)((value >> 16) & 0xFF), 
						(int)((value >> 8) & 0xFF), (int)(value & 0xFF), False);
	if (REGISTER_TRACE && VERBOSE_TRACE)
	    fprintf(stderr,"--> TEXT_COLOR = %ld\n",(int)flat->value[S_ALT_COLOR]);
    }

    if (flat->status[S_ALT_BACKGROUND] == S_INFERRED) {
	value = (int)flat->value[S_ALT_BACKGROUND];
/*
	flat->value[S_ALT_BACKGROUND] = rgb2ix( (int)(value >> 24), (int)((value >> 16) & 0xFF), 
						     (int)((value >> 8) & 0xFF), (int)(value & 0xFF), False);
	if (REGISTER_TRACE && VERBOSE_TRACE)
	    fprintf(stderr,"-> TEXT_BACKGROUND = %ld\n", flat->value[S_ALT_BACKGROUND]);
*/
    }

    /* math is a bit awkward */

    if (unit_p->element == TAG_MATH) {
	
	long sym_size = (long)flat->value[S_FONT_SIZE];
	HTList *l = str2list("symbol", "symbol");
	    
	math_normal_sym_font = GetFont(l, sym_size, FONT_WEIGHT_MEDIUM, FONT_STYLE_NORMAL, False);
	math_small_sym_font = GetFont(l, sym_size/2,FONT_WEIGHT_MEDIUM, FONT_STYLE_NORMAL, True);
	    
	math_normal_text_font = 0;
	math_small_text_font = 0;
	    
	math_bold_text_font = GetFont(l, sym_size/2, FONT_WEIGHT_BOLD, FONT_STYLE_NORMAL, True);
	math_italic_text_font = GetFont(l, sym_size/2, FONT_WEIGHT_NORMAL, FONT_STYLE_ITALIC, True);
    }

    HTList_destroy(l); /* DJB 17-jan-96 */

    return flat;
}


/*
  StyleSane is a debugging functions which tries to print out the
  content of a style sheet
*/

void StyleSane(StyleSheet *s)
{
    int i;
    StyleRule *r;

    fprintf(stderr,"\nstylesane:\n");

    if (!s)
	return;

    for (i=0; i < TAG_LAST; i++) {
	HTList *l = s->element[i];
	StyleSelector *sel;

	fprintf(stderr,"  element %d\n", i);
	while ( (r = (StyleRule *)HTList_nextObject(l)) ) {
	    fprintf(stderr,"    selector (");
	    sel = r->selector;
	    do {
		if (sel->unit.class)
		    fprintf(stderr,"%d.%s ",sel->unit.element, sel->unit.class);
		else
		    fprintf(stderr,"%d ",sel->unit.element);
	    } while ((sel = sel->ancestor));
	    fprintf(stderr,") ");

	    fprintf(stderr,"    property %d \tweight %d \tvalue ", r->property, r->weight);

	    switch (r->property) {
	      case S_FONT_FAMILY:
	      case S_ALT_FONT_FAMILY:
		{
		    HTList *ll = (HTList *)r->value;
		    char *family;

		    while ((family = (char *)HTList_nextObject(ll))) {
			fprintf(stderr,"%s+",family);
		    }
		    fprintf(stderr," ");
		}
		break;

	      case S_COLOR:
	      case S_BACKGROUND:
		fprintf(stderr,"#%x,%x,%x ",(int)(((int)r->value >> 16) & 0xFF),(int)(((int)r->value >> 8) & 0xFF),(int)(((int)r->value >> 0) & 0xFF));
		break;
	      default:
		fprintf(stderr,"%ld ", (long)r->value);
	    }
	    fprintf(stderr,"\n");
	}
    }
    fprintf(stderr,"stylesane end\n");
}





/* StyleAddRule adds a rule to the pool of style rules */

void StyleAddRule(StyleSheet *sheet, StyleSelector *selector, StyleProperty property, long value, int weight)
{
    StyleRule *r = NewStyleRule();

    r->selector = selector;
    r->property = property;
    r->value = value;
    r->weight = weight;

    if (!sheet->element[selector->unit.element])
	sheet->element[selector->unit.element] = HTList_new();
    HTList_addObject(sheet->element[selector->unit.element], (void *) r);
}



void StyleAddSimpleRule(StyleSheet *sheet, int element, StyleProperty property, long value, int weight)
{
    StyleSelector *s = NewStyleSelector();

    s->unit.element = element;
    StyleAddRule(sheet, s, property, value, weight);
}


/* Parse functions start here */


StyleProperty Property2enum(char *s)
{
    int l = strlen(s);

/*     s = TOLOWER(s);*/

    if(!strncmp(s,"padding",7))
	return S_PADDING;
    if(!strncmp(s,"bg", 2))
    {
	if((l == 8 )&&(!strcmp(s+3,"style")))
	    return S_BACKGROUND;
	if((l == 11 ) &&(!strcmp(s+3,"position")))
	    return S_BACKGROUND;
	if((l == 18 ) && (!strcmp(s+3,"blend-direction")))
	    return S_BACKGROUND_BLEND;
			  
    };
    if(!strncmp(s,"background", 10))
	return S_BACKGROUND;
    if (strncmp(s,"font", 4) == 0) {
	if (l == 4)
	    return S_FONT_SHORTHAND;
	if ((l == 9) && (strcmp(&s[5], "size") == 0))
	    return S_FONT_SIZE;
/*
	if ((l == 15) && (strcmp(&s[5], "size-index") == 0))
	    return S_FONT_SIZE_INDEX;
*/
	if ((l == 11) && (strcmp(&s[5], "family") == 0))
	    return S_FONT_FAMILY;
	if ((l == 11) && (strcmp(&s[5], "weight") == 0))
	    return S_FONT_WEIGHT;
	if ((l == 10) && (strcmp(&s[5], "style") == 0))
	    return S_FONT_STYLE;
	if ((l == 12) && (strcmp(&s[5], "leading") == 0))
	    return S_FONT_LEADING;
    }
    else if (strncmp(s,"letter", 6) == 0) {
	if ((l == 14) && (strcmp(&s[7], "spacing") == 0))
	    return S_TEXT_SPACING;
    }
     else if (strncmp(s,"word", 4) == 0) {
	if ((l == 12) && (strcmp(&s[5], "spacing") == 0))
	    return S_WORD_SPACING;
    }
    else if (strncmp(s,"text", 4) == 0) {
	if ((l == 10) && (strcmp(&s[5], "color") == 0))        /* historical */
	    return S_COLOR;
	if ((l == 15) && (strcmp(&s[5], "background") == 0))   /* historical */
	    return S_BACKGROUND;
	if ((l == 12) && (strcmp(&s[5], "spacing") == 0))
	    return S_TEXT_SPACING;
	if ((l == 9) && (strcmp(&s[5], "decoration") == 0))
	    return S_TEXT_DECORATION;
	if ((l == 9) && (strcmp(&s[5], "line") == 0))          /* historical */
	    return S_TEXT_DECORATION;
	if ((l == 13) && (strcmp(&s[5], "position") == 0))
	    return S_TEXT_POSITION;
	if ((l == 14) && (strcmp(&s[5], "transform") == 0))
	    return S_TEXT_TRANSFORM;
	if ((l == 11) && (strcmp(&s[5], "effect") == 0))
	    return S_TEXT_EFFECT;
	if ((l == 11) && (!strcmp(s+5, "indent")))
	    return S_INDENT;
	if ((l == 10) && (!strcmp(s+5, "align")))
	    return S_ALIGN;
    } else if (strncmp(s,"margin", 6) == 0) {
	if (l == 6)
	    return S_MARGIN_SHORTHAND;
	if ((l == 10) && (strcmp(&s[7], "top") == 0))
	    return S_MARGIN_TOP;
	if ((l == 12) && (strcmp(&s[7], "right") == 0))
	    return S_MARGIN_RIGHT;
	if ((l == 13) && (strcmp(&s[7], "bottom") == 0))
	    return S_MARGIN_BOTTOM;
	if ((l == 11) && (strcmp(&s[7], "left") == 0))
	    return S_MARGIN_LEFT;
    } else if (strncmp(s, "color", 5) == 0) {
      return S_COLOR;
    } else if (strncmp(s, "background", 10) == 0) {
      return S_BACKGROUND;
    } else if (strncmp(s,"alt", 3) == 0) {

	s += 4;
	l -= 4;
	if (strncmp(s,"font", 4) == 0) {
	    if (l == 4)
		return S_ALT_FONT_SHORTHAND;
	    if ((l == 9) && (strcmp(&s[5], "size") == 0))
		return S_ALT_FONT_SIZE;
/*
	    if ((l == 15) && (strcmp(&s[5], "size-index") == 0))
		return S_ALT_FONT_SIZE_INDEX;
*/
	    if ((l == 11) && (strcmp(&s[5], "family") == 0))
		return S_ALT_FONT_FAMILY;
	    if ((l == 11) && (strcmp(&s[5], "weight") == 0))
		return S_ALT_FONT_WEIGHT;
	    if ((l == 10) && (strcmp(&s[5], "style") == 0))
		return S_ALT_FONT_STYLE;
	    if ((l == 12) && (strcmp(&s[5], "leading") == 0))
		return S_ALT_FONT_LEADING;
	} else if (strncmp(s,"text", 4) == 0) {
	    if ((l == 10) && (strcmp(&s[5], "color") == 0))           /* historical */
		return S_ALT_COLOR;
	    if ((l == 15) && (strcmp(&s[5], "background") == 0))      /* historical */
		return S_ALT_BACKGROUND;
	    if ((l == 13) && (strcmp(&s[5], "spacing") == 0))
		return S_ALT_TEXT_SPACING;
	    if ((l == 9) && (strcmp(&s[5], "decoration") == 0))
		return S_ALT_TEXT_DECORATION;
	    if ((l == 9) && (strcmp(&s[5], "line") == 0))             /* historical */
		return S_ALT_TEXT_DECORATION;
	    if ((l == 13) && (strcmp(&s[5], "position") == 0))
		return S_ALT_TEXT_POSITION;
	    if ((l == 14) && (strcmp(&s[5], "transform") == 0))
		return S_ALT_TEXT_TRANSFORM;
	} else if (strncmp(s, "color", 5) == 0) {
	  return S_COLOR;
	} else if (strncmp(s, "background", 10) == 0) {
	  return S_BACKGROUND;
	}

	if (STYLE_TRACE & VERBOSE_TRACE)
	    fprintf(stderr,"can't chew *prop_peter %s\n", s);
	return S_UNKNOWN;

    }
    
    if(strcasecmp(s,"indent")==0)
	return S_INDENT;

    if (strcasecmp(s,"font.family")==0) {
	return S_FONT_FAMILY;
    } else if (strcasecmp(s,"font.size")==0) {
	return S_FONT_SIZE;
    } else if ( (strcasecmp(s,"font.color")==0) || (strcasecmp(s,"color.text")==0)) {
	return S_COLOR;
	
	
    } else if (strcasecmp(s,"oversize.font.family")==0) {
	return S_ALT_FONT_FAMILY;
    } else if (strcasecmp(s,"oversize.font.size")==0) {
	return S_ALT_FONT_SIZE;
    } else if (strcasecmp(s,"oversize.font.color")==0) {
	return S_ALT_COLOR;
	
    } else if (strcasecmp(s,"back.color")==0) {
	return S_BACKGROUND;
    } else if (strcasecmp(s,"color.background")==0) {   /* backwards compatibility */
	return S_BACKGROUND;
    } else {
	if (STYLE_TRACE & VERBOSE_TRACE)
	    fprintf(stderr,"can't chew *prop_peter %s\n", s);
	return S_UNKNOWN;
    }
}


int element_enum(char *s, int *token_class_p)
{
    int c, len;

    len = strlen(s);
    c = TOLOWER(*s);

    if (isalpha(c))
    {
        if (c == 'a')
        {
            if (len == 1 && strncasecmp(s, "a", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_ANCHOR;
            }

            if (len == 3 && strncasecmp(s, "alt", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_ALT;
            }

            if (len == 5 && strncasecmp(s, "added", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_ADDED;
            }

            if (len == 7 && strncasecmp(s, "address", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_ADDRESS;
            }

            if (len == 8 && strncasecmp(s, "abstract", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_ABSTRACT;
            }
        }
        else if (c == 'b')
        {
            if (len == 1)
            {
                *token_class_p = EN_TEXT;
                return TAG_BOLD;
            }

            if (len == 2 && strncasecmp(s, "br", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_BR;
            }

            if (len == 4 && strncasecmp(s, "body", len) == 0)
            {
                *token_class_p = EN_MAIN;
                return TAG_BODY;
            }

            if (len == 10 && strncasecmp(s, "blockquote", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_QUOTE;
            }

            if (len == 4 && strncasecmp(s, "base", len) == 0)
            {
                *token_class_p = EN_SETUP;
                return TAG_BASE;
            }
        }
        else if (c == 'c')
        {
            if (len == 4)
            {
                if (strncasecmp(s, "code", len) == 0)
                {
                    *token_class_p = EN_TEXT;
                    return TAG_CODE;
                }

                if (strncasecmp(s, "cite", len) == 0)
                {
                    *token_class_p = EN_TEXT;
                    return TAG_CITE;
                }
            }
            else if (len == 7 && (strncasecmp(s, "caption", len) == 0))/* howcome 3/2/95: = -> == after hint from P.M.Hounslow@reading.ac.uk */
            {
                *token_class_p = EN_BLOCK;
                return TAG_CAPTION;
            }
        }
        else if (c == 'd')
        {
            if (len == 3 && strncasecmp(s, "dfn", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_DFN;
            }

	    /* howcome 11/8/95: added upport for DIR */

            if (len == 3 && strncasecmp(s, "dir", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_UL;
            }


            if (len != 2)
                return 0;

            if (strncasecmp(s, "dl", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_DL;
            }

            if (strncasecmp(s, "dt", len) == 0)
            {
                *token_class_p = EL_DEFLIST;
                return TAG_DT;
            }

            if (strncasecmp(s, "dd", len) == 0)
            {
                *token_class_p = EL_DEFLIST;
                return TAG_DD;
            }
        }
        else if (c == 'e')
        {
            if (len == 2 && strncasecmp(s, "em", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_EM;
            }
        }
        else if (c == 'f')
        {
            if (len == 3 && strncasecmp(s, "fig", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_FIG;
            }
        }
        else if (c == 'h')
        {
            if (len == 4) {
		if (strncasecmp(s, "head", len) == 0)
		{
		    *token_class_p = EN_SETUP;
		    return TAG_HEAD;
		} 

		/* added by howcome 27/8/95 */

		else if (strncasecmp(s, "html", len) == 0)
		{
		    *token_class_p = EN_SETUP;
		    return TAG_HTML;
		}
	    }

            if (len != 2)
                return 0;

            *token_class_p = EN_HEADER;
            c = TOLOWER(s[1]);

            switch (c)
            {
                case '1':
                    return TAG_H1;
                case '2':
                    return TAG_H2;
                case '3':
                    return TAG_H3;
                case '4':
                    return TAG_H4;
                case '5':
                    return TAG_H5;
                case '6':
                    return TAG_H6;
                case 'r':
                    *token_class_p = EN_BLOCK;
                    return TAG_HR;
            }
        }
        else if (c == 'i')
        {
            if (len == 1)
            {
                *token_class_p = EN_TEXT;
                return TAG_ITALIC;
            }

            if (len == 3 && strncasecmp(s, "img", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_IMG;
            }

            if (len == 5 && strncasecmp(s, "input", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_INPUT;
            }

            if (len == 7 && strncasecmp(s, "isindex", len) == 0)
            {
                *token_class_p = EN_SETUP;
                return TAG_ISINDEX;
            }
        }
        else if (c == 'k')
        {
            if (len == 3 && strncasecmp(s, "kbd", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_KBD;
            }
        }
        else if (c == 'l')
        {
            if (len == 2 && strncasecmp(s, "li", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_LI;
            }

            if (len == 4 && strncasecmp(s, "link", len) == 0)
            {
                *token_class_p = EN_SETUP;
                return TAG_LINK;
            }

        }
        else if (c == 'm')
        {
            if (len == 4 && strncasecmp(s, "math", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_MATH;
            }

            if (len == 6 && strncasecmp(s, "margin", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_MARGIN;
            }

	    /* howcome 11/8/95: added MENU to be compatible with HTML2 */
            if (len == 4 && strncasecmp(s, "menu", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_UL;
            }


        }
        else if (c == 'n')
        {
            if (len == 4 && strncasecmp(s, "note", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_NOTE;
            }
        }
        else if (c == 'o')
        {
            if (len == 2 && strncasecmp(s, "ol", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_OL;
            }

            if (len == 6 && strncasecmp(s, "option", len) == 0)
            {
                *token_class_p = EN_TEXT;  /* kludge for error recovery */
                return TAG_OPTION;
            }
        }
        else if (c == 'p')
        {
            if (len == 1)
            {
                *token_class_p = EN_BLOCK;
                return TAG_P;
            }

            if (len == 3 && strncasecmp(s, "pre", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_PRE;
            }
        }
        else if (c == 'q')
        {
            if (len == 1)
            {
                *token_class_p = EN_TEXT;
                return TAG_Q;
            }

            if (len == 5 && strncasecmp(s, "quote", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_QUOTE;
            }
        }
        else if (c == 'r')
        {
            if (len == 7 && strncasecmp(s, "removed", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_REMOVED;
            }
        }
        else if (c == 's')
        {
            if (len == 1)
            {
                *token_class_p = EN_TEXT;
                return TAG_STRIKE;
            }

            if (len == 3 && strncasecmp(s, "sup", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_SUP;
            }

            if (len == 3 && strncasecmp(s, "sub", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_SUB;
            }

            if (len == 4 && strncasecmp(s, "samp", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_SAMP;
            }

            if (len == 5 && strncasecmp(s, "small", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_SMALL;
            }

            if (len == 6 && strncasecmp(s, "strong", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_STRONG;
            }

            if (len == 6 && strncasecmp(s, "select", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_SELECT;
            }

            if (len == 6 && strncasecmp(s, "strike", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_STRIKE;
            }

	    /* howcome 26/2/95 */

            if (len == 5 && strncasecmp(s, "style", len) == 0)
            {
                *token_class_p = EN_SETUP;
                return TAG_STYLE;
            }

        }
        else if (c == 't')
        {
            if (len == 5 && strncasecmp(s, "title", len) == 0)
            {
                *token_class_p = EN_SETUP;
                return TAG_TITLE;
            }

            if (len == 2 && strncasecmp(s, "tt", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_TT;
            }

            if (len == 2 && strncasecmp(s, "tr", len) == 0)
            {
                *token_class_p = EN_TABLE;
                return TAG_TR;
            }

            if (len == 2 && strncasecmp(s, "th", len) == 0)
            {
                *token_class_p = EN_TABLE;
                return TAG_TH;
            }

            if (len == 2 && strncasecmp(s, "td", len) == 0)
            {
                *token_class_p = EN_TABLE;
                return TAG_TD;
            }

            if (len == 5 && strncasecmp(s, "table", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_TABLE;
            }

            if (len == 8 && strncasecmp(s, "textarea", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_TEXTAREA;
            }
        }
        else if (c == 'u')
        {
            if (len == 1)
            {
                *token_class_p = EN_TEXT;
                return TAG_UNDERLINE;
            }

            if (len == 2 && strncasecmp(s, "ul", len) == 0)
            {
                *token_class_p = EN_LIST;
                return TAG_UL;
            }
        }
        else if (c == 'v')
        {
            if (len == 3 && strncasecmp(s, "var", len) == 0)
            {
                *token_class_p = EN_TEXT;
                return TAG_VAR;
            }
        }
        else if (c == 'x')
        {
            if (len == 3 && strncasecmp(s, "xmp", len) == 0)
            {
                *token_class_p = EN_BLOCK;
                return TAG_PRE;
            }
        }
    }

    *token_class_p = EN_UNKNOWN;
    return UNKNOWN; /* unknown tag */
}


StyleSelector *ParseSelector(char *str)
{
    char *p;
    char *elem_marker, *subelem_marker;
    StyleSelector *selector = NULL, *prev_selector = NULL;
    int token_class;

    p = str_tok(str,"/() \t",&elem_marker);
    do {
	int element;
	char *class;

	p = str_tok(p, ".",&subelem_marker);
	element = element_enum(p, &token_class);

	if (strcmp(p, "*") == 0) {
	    element = TAG_HTML;
	} else if (strcasecmp(p, "$HTML-SOURCE") == 0) {
	    element = TAG_HTML_SOURCE;
	}
	
	if (strcmp(p, "HTML") == 0) {
	    element = TAG_HTML;
	} else if (strcasecmp(p, "$HTML-SOURCE") == 0) {
	    element = TAG_HTML_SOURCE;
	}

	if (element == UNKNOWN) {
	    return NULL;
	}

	class = str_tok(NULL," \t",&subelem_marker);
	
	selector = NewStyleSelector();
	selector->unit.element = element;
	if (class && *class)
	    selector->unit.class = strdup(class);
	selector->ancestor = prev_selector;
	prev_selector = selector;
	
    } while ( (p = str_tok(NULL, "() \t",&elem_marker)) );

    return selector;
}


BG_Style *StyleGetBackground(void *value_p, char *str)
{
    BG_Style *bg_style;
    Image *image;
    char *tmp;
    int is_numeric;

    if (value_p) {
	bg_style = (BG_Style *) value_p;
    } else  { 
	bg_style = (BG_Style *) calloc(1, sizeof(BG_Style));
	bg_style->flag = S_BACKGROUND_X_REPEAT | S_BACKGROUND_Y_REPEAT;
	bg_style->x_pos = 50;
	bg_style->y_pos = 50;
    } 
    if(!strcmp(str,":"))
	return (bg_style);
    
    if (str[0] == '#') {
	if (strlen(str) == 4) {
	    bg_style->r = hex2byte(str[1]) * 17;
	    bg_style->g = hex2byte(str[2]) * 17;
	    bg_style->b = hex2byte(str[3]) * 17;
	    bg_style->flag |= S_BACKGROUND_COLOR;
	    /*		    fprintf(stderr,"%x %x %x -> %d\n", r, g, b, *value_p);*/
	} else if (strlen(str) == 7) {
	    bg_style->r = hex2byte(str[1]) * 16 + hex2byte(str[2]);
	    bg_style->g = hex2byte(str[3]) * 16 + hex2byte(str[4]);
	    bg_style->b = hex2byte(str[5]) * 16 + hex2byte(str[6]);
	    bg_style->flag |= S_BACKGROUND_COLOR;
	    /*		    fprintf(stderr,"%x %x %x -> %d\n", r, g, b, *value_p);*/
	}
    } else if (str[0] == '"')
    {
	if ((image = GetImage(str+1, strlen(str)-2, CurrentDoc->pending_reload))) {
	    bg_style->image = image;
	    bg_style->flag |= S_BACKGROUND_IMAGE;
	}
    } else {		/* try looking up the name */
	if (!strcasecmp(str,"fixed"))
	    bg_style->flag |= S_BACKGROUND_FIXED;
	else {
	    if (!strcasecmp(str,"repeat-x"))
		bg_style->flag &= ~S_BACKGROUND_Y_REPEAT;
	    else {
		if (!strcasecmp(str,"repeat-y"))
		    bg_style->flag &= ~S_BACKGROUND_X_REPEAT;
		else {
		    if (!strcasecmp(str,"no-repeat"))
			bg_style->flag &= ~(S_BACKGROUND_X_REPEAT|S_BACKGROUND_Y_REPEAT);
		    else
		    {
			is_numeric = TRUE;
			tmp = str;
			while(*tmp&&is_numeric)
			{
			    is_numeric = ((*tmp>='0') && (*tmp <= '9')) || (*tmp=='%');
			    tmp++;
			};
			if(is_numeric)
			{
			    if(bg_style->flag & S_BACKGROUND_ORIGIN)
				bg_style->y_pos = atoi(str);
			    else
			    {
				bg_style->flag |= S_BACKGROUND_ORIGIN;
				bg_style->x_pos = atoi(str);
				bg_style->y_pos = bg_style->x_pos;
			    }
			}
			else
			{
			    long color = ParseColor(str);
			    bg_style->r = (color >> 16) & 0xFF;
			    bg_style->g = (color >>  8) & 0xFF;
			    bg_style->b = color & 0xFF;
			    bg_style->flag |= S_BACKGROUND_COLOR;
			}
		    }
		}
	    }
	}
    }
    return (bg_style);
}


/* ?? */
static Bool ParseAssignment(char *s, StyleProperty *prop_p, long *value_p, int *level_p, char delimiter)
{
    char *str;
    char *elem, *value;
    char first_str_tok[8], second_str_tok[8];

    if (!s)
	return False;

    sprintf(first_str_tok,"%c \t", delimiter);
    str = str_tok(s,first_str_tok,&elem);
    if (!str)
	return False;
    
    *prop_p = Property2enum(str);

    if (*prop_p == S_UNKNOWN)
	return False;
    
    if (STYLE_TRACE & VERBOSE_TRACE)
	fprintf(stderr,"StyleChew: *prop_p = %d\n",*prop_p);
    

    /* get everything up to '!' */
    
    str = str_tok(NULL,"!",&elem);
    if (!str)
	return False;
    sprintf(first_str_tok,"%c,;& \t",delimiter);
    str = str_tok(str,first_str_tok,&value);
    
    while (str) {
	switch (*prop_p) {
	  case S_FONT_FAMILY:
	  case S_ALT_FONT_FAMILY:
	    if (! *value_p)
		*value_p = (long) HTList_new();
	    HTList_addObjectFirst((HTList *)*value_p, (void *)strdup(str)); /* ughh */
	    break;

	  case S_FONT_SIZE:
	  case S_ALT_FONT_SIZE:
	  case S_FONT_LEADING:
	  case S_ALT_FONT_LEADING:
	    if (strcasecmp(str,"xx-large") == 0)
		*value_p = (long)21;
	    else if (strcasecmp(str,"x-large") == 0)
		*value_p = (long)17;
	    else if (strcasecmp(str,"large") == 0)
		*value_p = (long)14;
	    else if (strcasecmp(str,"medium") == 0)
		*value_p = (long)12;
	    else if (strcasecmp(str,"small") == 0)
		*value_p = (long)10;
	    else if (strcasecmp(str,"x-small") == 0)
		*value_p = (long)9;
	    else if (strcasecmp(str,"xx-small") == 0)
		*value_p = (long)8;
	    else if (strstr(str,"px")) {
		*value_p = (long)atoi(str);
	    } else {		/* pt being the default sixe unit */
		*value_p = (long)pt2px((double)atof(str));
	    }
	    break;

	  case S_FONT_WEIGHT:
	  case S_ALT_FONT_WEIGHT:

	    if (strcasecmp(str,"light") == 0)
		*value_p = (long)FONT_WEIGHT_LIGHT;
	    else if (strcasecmp(str,"medium") == 0)
		*value_p = (long)FONT_WEIGHT_MEDIUM;
	    else if (strcasecmp(str,"demibold") == 0)
		*value_p = (long)FONT_WEIGHT_DEMIBOLD;
	    else if (strcasecmp(str,"bold") == 0)
		*value_p = (long)FONT_WEIGHT_BOLD;
	    break;

	  case S_FONT_STYLE:
	  case S_ALT_FONT_STYLE:

	    if (strncasecmp(str,"italic", 6) == 0) /* italics */
		*value_p = (long)FONT_STYLE_ITALIC;
	    else if (strcasecmp(str,"roman") == 0)
		*value_p = (long)FONT_STYLE_ROMAN;
	    else if (strcasecmp(str,"small-caps") == 0)
		*value_p = (long)FONT_STYLE_SMALL_CAPS;

	    break;

	  case S_TEXT_EFFECT:

	    if (strcasecmp(str,"initial-cap") == 0)
		*value_p = (long)TEXT_EFFECT_INITIAL_CAP;
	    else if (strcasecmp(str,"drop-cap") == 0)
		*value_p = (long)TEXT_EFFECT_DROP_CAP;
	    else if (strcasecmp(str,"alt-firstline") == 0)
		*value_p = (long)TEXT_EFFECT_ALT_FIRSTLINE;
	    break;


	  case S_TEXT_DECORATION:

	    if (strcasecmp(str,"blink") == 0)
		*value_p = (long)TEXT_LINE_BLINK;
	    break;

	  case S_MARGIN_LEFT:
	  case S_MARGIN_RIGHT:
	  case S_MARGIN_TOP:
   	  case S_MARGIN_BOTTOM:
 	  case S_INDENT:
	  case S_TEXT_SPACING:
	      *value_p = (long)(atoi(str));
	      break;
	 
	  case S_ALIGN:
	    if (strcasecmp(str,"left") == 0) {	
		*value_p = (long)ALIGN_LEFT;
	    } else if (strcasecmp(str,"center") == 0) {	
		*value_p = (long)ALIGN_CENTER;
	    } else if (strcasecmp(str,"right") == 0) {	
		*value_p = (long)ALIGN_RIGHT;
	    } else if (strcasecmp(str,"justify") == 0) {
		*value_p = (long)ALIGN_JUSTIFY;
	    } else
		return False;
	    break;

	  case S_COLOR:
	  case S_ALT_COLOR:
	    if (str[0] == '#') {
		Byte r, g, b;

		if (strlen(str) == 4) {
		    r = hex2byte(str[1]) * 17;
		    g = hex2byte(str[2]) * 17;
		    b = hex2byte(str[3]) * 17;
		    *value_p = ((r<<16) | (g<<8) | b);
/*		    fprbytef(stderr,"%x %x %x -> %d\n", r, g, b, *value_p);*/
		} else if (strlen(str) == 7) {
		    r = hex2byte(str[1]) * 16 + hex2byte(str[2]);
		    g = hex2byte(str[3]) * 16 + hex2byte(str[4]);
		    b = hex2byte(str[5]) * 16 + hex2byte(str[6]);
		    *value_p = ((r<<16) | (g<<8) | b);
/*		    fprintf(stderr,"%x %x %x -> %d\n", r, g, b, *value_p);*/
		}
	    } else {		/* try looking up the name */
		*value_p = (long)ParseColor(str);
	    }
	    break;	    
  	  case S_PADDING:
	      *value_p = (long)atoi(str); /* only x & y & i & j */
	      break;
	  case S_BACKGROUND:
	  case S_ALT_BACKGROUND:
	      *value_p = (long) StyleGetBackground(*value_p, str);
	      break;
	  default:
	    if (STYLE_TRACE & VERBOSE_TRACE)
		fprintf(stderr,"can't chew value: %s\n", str);
	    return False;
	    break;
	}
	sprintf(second_str_tok,"%c,& \t",delimiter);
	str = str_tok(NULL,second_str_tok,&value);
    }
    /*  level_declaration: */

    *level_p = I_NORMAL;

    str = str_tok(NULL,"! \t;",&elem);
    if (str) {
	if (strcasecmp(str,"important") == 0) {
	    *level_p = I_IMPORTANT;
	}
	else if (strcasecmp(str,"insist") == 0) {
	    *level_p = I_INSIST;
	}
	else if (strcasecmp(str,"legal") == 0) {
	    *level_p = I_LEGAL;
	}
    }

    return True;
}


/*
  StyleChew is the main entry point to the CSS parser. 

  *sheet is a pointer to a StyleSheet structure into which the
  assignments will be added

  *str is a pointer to a string containing CSS syntax. Th pointer wil
  not be freed,a dn the content will remain unchanged.

  base_weight identifies the source of the style sheet, author or
  reader

*/


void StyleChew(StyleSheet *sheet, char *str, int base_weight)
{
    char *sheet_marker, *address_marker, *assignment_marker;
    char *p, *q, *r, *s;
    StyleProperty property;
    int level;
    int weight;
    long value;
    void *bg_save;
    int bg_weight=0;
    HTList *l, *selectors;
    StyleSelector *selector;
    char begin_char,end_char,set_char,*tmp_char; /* used for backward compatibility --Spif 23-Nov-95 */
    char delim_char;
    char str_tok_begin[2], str_tok_set[2], str_tok_end[2];

    if (STYLE_TRACE & VERBOSE_TRACE)
	fprintf(stderr,"\nStyleChew\n");

    /* h1, p: font-family = helvetica times, font-size = 12pt; */

    if (!str)
	return;

    begin_char = ':' ; 
    set_char   = '=' ;
    delim_char = ',';
    end_char   = '\n';
    for(tmp_char = str; *tmp_char ; tmp_char++)  /* figure out what kind of notation, old or new one --Spif 23-Nov-95 */
	if(*tmp_char == '{')
	{
	    begin_char = '{';
	    set_char   = ':';
	    delim_char = ';';
	    end_char   = '}';
	};
    *str_tok_end = end_char;
    *str_tok_begin = begin_char;
    *str_tok_set = set_char;
    str_tok_begin[1] = 0;
    str_tok_set[1] = 0;
    str_tok_end[1] = 0;

    s = strdup(str);  /* we will modify the string so we make a copy */
    
    while(tmp_char = strstr(s, "--"))
    {
	while(*tmp_char && (*tmp_char!='\n'))
	    *tmp_char++=' ';
    };
    if(begin_char=='{')
	for(tmp_char = s; *tmp_char ;tmp_char++)
	    if(*tmp_char=='\n')
		*tmp_char = ' ';

    p = str_tok(s,str_tok_end ,&sheet_marker);
    if (!p)
	return;

    do {

	/* first screen out the comments */

	if (p[0] == '#')
	    continue;

	if ((q = strstr(p, "--")))
	    *q = 0;


	/* then see if the "left side" (before ':') is specified */

	if ((q = strchr(p, begin_char)) && (r = strchr(p,set_char)) && (q < r)) {

	    /* yes, the left side is specified, nail it */
	    p = str_tok(p, str_tok_begin, &address_marker);

	    if (strchr(p, '/'))
		continue;
	    if (STYLE_TRACE)
		fprintf(stderr,"(address) %s -> ", p);

	    selectors = HTList_new();

	    p = str_tok(p, ",;", &address_marker);
	    do {
		StyleSelector *s;

		if ((s = ParseSelector(p))) {
		    if (STYLE_TRACE) {
			StyleSelector *sc = s;
			do {
			    fprintf(stderr,"%d-",sc->unit.element);
			} while((sc = sc->ancestor));
		    }

		    HTList_addObject(selectors, (void *) s);
		}
		
	    } while ((p = str_tok(NULL,",;",&address_marker)));

	    p = q + 1;
	} else {
	    if (STYLE_TRACE)
		fprintf(stderr,"StyleChew, left side not specified..\n");
	    continue;
	}

	/* now we know what elements we are addressing */

	p = str_tok(p, ";,", &assignment_marker);
	bg_save = NULL;
	do {
	    if (STYLE_TRACE)
		fprintf(stderr,"(assignment) %s ", p);
	   
	    value = (long)bg_save;  /* this hack is only for keeping bg_style during the parsing of one element --Spif 23-Nov-95 */
	    if (ParseAssignment(p, &property, &value, &level, set_char)) 
	    {
		if (base_weight == S_USER)
		    weight = base_weight + S_USER_DELTA * level;
		else
		    weight = base_weight + S_DOC_DELTA * level;
		if(property == S_BACKGROUND)
		{
		    bg_save = (void *)value;
		    bg_weight = weight;
		}
		else
		{
		    l = selectors;
		    while ((selector = (StyleSelector *)HTList_nextObject(l)) ) 
		    {
			if (STYLE_TRACE) 
			{
			    StyleSelector *sc = selector;
			    fprintf(stderr,"\nnext selector ");
			    do 
			    {
				fprintf(stderr,"%d-",sc->unit.element);
			    } while((sc = sc->ancestor));
			    fprintf(stderr,"\n");
			}
			StyleAddRule(sheet, selector, property, value, weight);
		    }
		}
	    }
	} while ( (p = str_tok(NULL,";,",&assignment_marker)) ); 
	if(bg_save)
	{
	    l = selectors;
	    while ((selector = (StyleSelector *)HTList_nextObject(l)) ) 
	    {
		if (STYLE_TRACE) 
		{
		    StyleSelector *sc = selector;
		    fprintf(stderr,"\nnext selector ");
		    do 
		    {
			fprintf(stderr,"%d-",sc->unit.element);
		    } while((sc = sc->ancestor));
		    fprintf(stderr,"\n");
		}
		StyleAddRule(sheet, selector, S_BACKGROUND,(long)bg_save, bg_weight);
	    }
	};
	if (STYLE_TRACE)
	    fprintf(stderr,"\n");

/*	
	l = selectors;
	while ( (selector = (StyleSelector *)HTList_nextObject(l)) ) {
	    if (STYLE_TRACE && VERBOSE_TRACE)
		fprintf(stderr,"freeing selector\n");
	    FreeStyleSelector(selector);
	}
	HTList_delete(selectors);
*/
    } while ( (p = str_tok(NULL, str_tok_end, &sheet_marker)) ); 

    Free(s);
}

/*
  StyleGetInit returns an initialized style sheet, typically the application's default 
*/

StyleSheet *StyleGetInit()
{
    StyleSheet *sheet = NewStyleSheet();

    /* set toplevel fallback values */
    StyleAddSimpleRule(sheet, TAG_HTML, S_FONT_FAMILY,		(long)str2list("helvetica", "charter"),	S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_FONT_SIZE, 		12,						S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_FONT_WEIGHT, 		FONT_WEIGHT_MEDIUM, 			      	S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_FONT_STYLE, 		0,						S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_FONT_LEADING, 	0,						S_FALLBACK - S_ANY);

    StyleAddSimpleRule(sheet, TAG_HTML, S_COLOR, 		(100 << 0),		S_FALLBACK - S_ANY);  /* i.e. a dark blue */
    StyleAddSimpleRule(sheet, TAG_HTML, S_ALT_COLOR, 		(100 << 0),		S_FALLBACK - S_ANY);  /* i.e. a dark blue */
    StyleAddSimpleRule(sheet, TAG_HTML, S_ALT_BACKGROUND,	(long)StyleGetBackground(NULL, "#AAA"),	S_FALLBACK - S_ANY); /* i.e. transparent */
    
    StyleAddSimpleRule(sheet, TAG_HTML, S_TEXT_SPACING, 	0, 		S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_MARGIN_LEFT,	 	4,		S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_MARGIN_RIGHT,	 	4,		S_FALLBACK - S_ANY);
    StyleAddSimpleRule(sheet, TAG_HTML, S_MARGIN_TOP,	 	4,		S_FALLBACK - S_ANY);

    StyleAddSimpleRule(sheet, TAG_HTML, S_ALIGN,		 	ALIGN_LEFT,    	S_FALLBACK - S_ANY);

    /* set per-tag fallback values -- exception to the normal fallback values */

    StyleAddSimpleRule(sheet, TAG_H1, S_FONT_FAMILY, 		(long)str2list("charter", "times"), 		S_FALLBACK); 
    StyleAddSimpleRule(sheet, TAG_H1, S_FONT_SIZE, 		24, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H1, S_FONT_WEIGHT, 		FONT_WEIGHT_BOLD,				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H1, S_MARGIN_LEFT,	 	5, 						S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_H2, S_FONT_FAMILY, 		(long)str2list("charter", "times"), 		S_FALLBACK); 
    StyleAddSimpleRule(sheet, TAG_H2, S_FONT_SIZE, 		18, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H2, S_FONT_WEIGHT, 		FONT_WEIGHT_BOLD, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H2, S_MARGIN_LEFT,	 	5, 						S_FALLBACK);
    
    StyleAddSimpleRule(sheet, TAG_H3, S_FONT_FAMILY, 		(long)str2list("times", "charter"), 		S_FALLBACK); 
    StyleAddSimpleRule(sheet, TAG_H3, S_FONT_SIZE, 		14, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H2, S_FONT_WEIGHT, 		FONT_WEIGHT_BOLD, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H3, S_MARGIN_LEFT, 		5, 						S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_H4, S_FONT_FAMILY, 		(long)str2list("charter","times"), 		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H4, S_FONT_SIZE, 		12, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H4, S_FONT_WEIGHT,   		FONT_WEIGHT_BOLD, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H4, S_MARGIN_LEFT, 		5, 						S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_H5, S_FONT_FAMILY, 		(long)str2list("charter","times"), 		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H5, S_FONT_SIZE, 		12, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H5, S_FONT_WEIGHT,   		FONT_WEIGHT_BOLD, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H5, S_MARGIN_LEFT, 		5, 						S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_H6, S_FONT_FAMILY, 		(long)str2list("charter","times"), 		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H6, S_FONT_SIZE, 		12, 						S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H6, S_FONT_WEIGHT,   		FONT_WEIGHT_BOLD, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_H6, S_MARGIN_LEFT, 		5, 						S_FALLBACK);

    /*    StyleAddSimpleRule(sheet, TAG_LI, S_MARGIN_LEFT, 	15, 			S_FALLBACK);*/

    StyleAddSimpleRule(sheet, TAG_DL, S_MARGIN_LEFT, 		15,	 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_UL, S_MARGIN_LEFT, 		15, 				S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_OL, S_MARGIN_LEFT, 		15, 				S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_DT, S_FONT_WEIGHT, 		FONT_WEIGHT_BOLD, 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_DD, S_FONT_WEIGHT, 		FONT_WEIGHT_MEDIUM, 			S_FALLBACK);
 
    StyleAddSimpleRule(sheet, TAG_ADDRESS, 	S_ALIGN,	ALIGN_RIGHT,		S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_MATH, 	S_FONT_SIZE, 	14,	 		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_SMALL, 	S_FONT_SIZE, 	8, 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_SUB, 		S_FONT_SIZE, 	8, 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_SUP, 		S_FONT_SIZE, 	8, 			S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_STRONG, 	S_FONT_WEIGHT,	FONT_WEIGHT_BOLD,		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_BOLD, 	S_FONT_WEIGHT, 	FONT_WEIGHT_BOLD,		S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_EM, 		S_FONT_STYLE,	FONT_STYLE_ITALIC,		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_ITALIC, 	S_FONT_STYLE,	FONT_STYLE_ITALIC,		S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_PRE, 		S_FONT_FAMILY,	(long)str2list("lucidasanstypewriter", "courier"),	S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_TT, 		S_FONT_FAMILY,	(long)str2list("lucidasanstypewriter", "courier"),	S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_CODE, 	S_FONT_FAMILY,	(long)str2list("lucidasanstypewriter", "courier"),	S_FALLBACK);

    /* set default style for TAG_ABSTRACT */

    StyleAddSimpleRule(sheet, TAG_ABSTRACT, S_FONT_STYLE, 	FONT_STYLE_ITALIC,		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_ABSTRACT, S_FONT_WEIGHT, 	FONT_WEIGHT_BOLD,		S_FALLBACK);

    StyleAddSimpleRule(sheet, TAG_ABSTRACT, S_MARGIN_RIGHT, 	40,			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_ABSTRACT, S_MARGIN_LEFT,	40, 			S_FALLBACK);

    /* set default style for TAG_QUOTE */

    StyleAddSimpleRule(sheet, TAG_QUOTE, S_FONT_STYLE, 		FONT_STYLE_ITALIC,		S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_QUOTE, S_MARGIN_RIGHT, 	30, 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_QUOTE, S_MARGIN_LEFT, 	50, 			S_FALLBACK);

    /* set style for tables */

    StyleAddSimpleRule(sheet, TAG_TH, 	S_FONT_WEIGHT, 		FONT_WEIGHT_BOLD, 			S_FALLBACK);
/*
    StyleAddSimpleRule(sheet, TAG_TH, 	S_ALIGN, 		ALIGN_CENTER, 			S_FALLBACK);
    StyleAddSimpleRule(sheet, TAG_TD, 	S_ALIGN, 		ALIGN_CENTER, 			S_FALLBACK);
*/
    /* set style for HR */

    StyleAddSimpleRule(sheet, TAG_HR, 	S_COLOR, 		(255 << 24),		S_FALLBACK);

    return sheet;
}

void StyleParse()
{
    if (STYLE_TRACE)
	fprintf(stderr,"StyleParse\n");

    if (!CurrentDoc->head_style && !CurrentDoc->link_style) {
	CurrentDoc->style = NULL; /* i.e. style->default */
	return;
    }

    if (CurrentDoc->user_style) {

	/* this document contains the user's style sheet if it exists */

	Announce("applying personal style sheet.. ");

/*	rgbClear();*/

	if (context->style) {
	    FreeStyleSheet(context->style);
	    context->style = StyleGetInit();
	}

	StyleChew(context->style, CurrentDoc->head_style, S_USER);
	StyleChew(context->style, CurrentDoc->link_style, S_USER);
	CurrentDoc->style = context->style;
	if (STYLE_TRACE) {
	    fprintf(stderr,"Stylesane: context->style\n");
	    StyleSane(context->style);
	}
	return;
    }

    /* this is a normal incoming document with style sheet attached */

    Announce("applying document's style sheet.. ");

    rgbClear();
    CurrentDoc->style = StyleCopy(context->style);
    StyleChew(CurrentDoc->style, CurrentDoc->head_style, S_USER);
    StyleChew(CurrentDoc->style, CurrentDoc->link_style, S_USER);
    if (STYLE_TRACE) {
	fprintf(stderr,"Stylesane: CurrentDoc->style\n");
	StyleSane(context->style);
    }

    Announce("applying document's style sheet.. done");
}


/*
void StyleWindowChange()
{
    fprintf(stderr,"StyleWindowChange w %d h %d\n", win_width, win_height);
}
*/

/* StyleZoom is a bit too Arena-specific */

void StyleZoomChange(double f)
{
    Announce("loading fonts..");
    ShowBusy();

    lens_factor *= f;
    DisplaySizeChanged(1);
    Redraw(0,0,WinWidth,WinHeight + WinTop); 
    Announce("%s",CurrentDoc->url);
    HideBusy();
}


char *StyleLoad(char *href, int hreflen, BOOL reload)
{
    Doc *doc = NULL;

    /* check for null name */

    doc = GetInline(href, hreflen, reload);
    if (doc) {
/*	fprintf(stderr,"StyleLoad succedded ??\n");*/
	return(doc->content_buffer);
    }
    return NULL;
}

/* 
  StyleGet is used by the UA to pull values out of a flattened stylesheet
*/


long StyleGet(StyleProperty property)
{
    if (REGISTER_TRACE && VERBOSE_TRACE)
	fprintf(stderr,"StyleGet: property %d\n", property);
    if ((REGISTER_TRACE) && (current_flat->status[property] == S_UNSET))
	fprintf(stderr, "StyleGet, property not set %d\n", property);

    switch (property) {
       case S_SMALL_CAPS_FONT: /* this one is expensive and not often used */
	if (current_flat->status[S_SMALL_CAPS_FONT] != S_INFERRED) {
	    current_flat->value[S_SMALL_CAPS_FONT] = (void *)
		GetFont((HTList *)current_flat->value[S_FONT_FAMILY], 
			(int)current_flat->value[S_FONT_SIZE] * 4 / 5,
			(int)current_flat->value[S_FONT_WEIGHT], 
			(int)current_flat->value[S_FONT_STYLE], True);
	    current_flat->status[S_FONT] = S_INFERRED;
	    current_flat->weight[S_SMALL_CAPS_FONT] = current_flat->weight[S_FONT];
	}
	return(current_flat->value[S_SMALL_CAPS_FONT]);
	break;
      default:
	return (current_flat->value[property]);
    }
    return 0;
}

void StyleClearDoc()
{
    HTList *l = style_stack;
    StyleStackElement *sel;

    while ( (sel = (StyleStackElement *) HTList_nextObject(l) )) {
	Free(sel->flat);
    }

    HTList_destroy(style_stack);
    style_stack = NULL;
    current_flat = NULL;
}



/* formatting functions, here starts what should become format.c */

StyleStackElement *NewStackElement()
{
    return (StyleStackElement *)calloc(1,sizeof(StyleStackElement));
}

/* 
  FormatElementStart is called when a new element (implied or not) is
  started. It will call StyleEval to flatten a set of properties (called
  a stack element) to be returned by StyleGet. The stack element is then 
  pushed onto the stack.
*/

extern void Push(void *);
extern void *Pop();

void FormatElementStart(int element, char *class, int class_len)
{
    Frame *new_frame, *old_frame;
    
    StyleStackElement *stack_el = NewStackElement();
    StyleSheet *sheet = (CurrentDoc->style ? CurrentDoc->style : context->style);

    if ((element < 0) || (element >= TAG_LAST)) {
      fprintf(stderr,"FormatElementStart: element = %d\n",element);
      element = TAG_P; /* ugly */
    }

    current_sheet = sheet;

    if (REGISTER_TRACE)
	fprintf(stderr,"\nFormatElementStart: %d\n",element);

    if (!style_stack)
	style_stack = HTList_new();

    stack_el->unit.element = element;
    if (class)
	stack_el->unit.class = strndup(class, class_len);
    stack_el->flat = StyleEval(sheet, &stack_el->unit, style_stack);

    if (REGISTER_TRACE) {
	StyleProperty i;
	for (i=S_UNKNOWN; i<S_NPROPERTY; i++) 
	    switch (i) {
	      case S_FONT_FAMILY:
		{
		    char *fn;
		    HTList *l = (HTList *)stack_el->flat->value[i];

		    fprintf(stderr,"(%d ",i);
		    while ((fn = (char *)HTList_nextObject(l))) {
			fprintf(stderr,"%s ",fn);
		    }
		    fprintf(stderr,")");
		}
		break;
	      default:
		fprintf(stderr,"(%d %ld)",i, stack_el->flat->value[i]);
	    }
	fprintf(stderr,"\n");
    }
		
    HTList_addObject(style_stack, (void *)stack_el); 
    current_flat = stack_el->flat;
    StyleSetFlag(S_MARGIN_TOP_FLAG, (int)StyleGet(S_MARGIN_TOP));
    if(element == TAG_P)
    { 
	StyleSetFlag(S_INDENT_FLAG, (int)StyleGet(S_INDENT));
	StyleSetFlag(S_LEADING_FLAG, TRUE);
    };

    if(element == TAG_HTML || element == TAG_HTML_SOURCE || 
	(/*element != TAG_TABLE && */
	 element != TAG_ABSTRACT &&
	 element != TAG_BLOCKQUOTE && element != TAG_CAPTION &&
	 element != TAG_NOTE && element != TAG_PRE &&
	 element != TAG_QUOTE))  
	return;
    if(prepass||damn_table)
	return;
    old_frame = (Frame *)Pop();
    Push(old_frame);
    if(paintStartLine>0)
	Push((Frame *)1);
    else
    {
	new_frame = (Frame *)calloc(1, sizeof(Frame));
	PixOffset += StyleGet(S_PADDING)+ StyleGet(S_MARGIN_TOP); /* must be S_PADDING_TOP */
	new_frame->offset = PixOffset;
	new_frame->leftmargin = StyleGet(S_PADDING);
	new_frame->rightmargin = StyleGet(S_PADDING);
	new_frame->indent = old_frame->indent + StyleGet(S_MARGIN_LEFT);
	new_frame->width = old_frame->width - StyleGet(S_MARGIN_LEFT) - StyleGet(S_MARGIN_RIGHT);
	new_frame->style = 0;
	new_frame->border = 0;
#ifdef STYLE_COLOR_BORDER
	new_frame->cb_ix = 0;
#else
	new_frame->cb_ix = 0;
#endif
	new_frame->flow = old_frame->flow; /* StyleGet(S_ALIGN) */
	new_frame->next = new_frame->child = NULL;
	new_frame->box_list = NULL;
	PrintBeginFrame(new_frame);
	Push(new_frame);
    }
#if 0
    new_frame->offset = PixOffset;
    new_frame->indent = (old_frame ?  old_frame->leftmargin + StyleGet(S_MARGIN_LEFT) : StyleGet(S_MARGIN_LEFT));
    new_frame->width = (old_frame ?  old_frame->width - StyleGet(S_MARGIN_LEFT) - StyleGet(S_MARGIN_RIGHT) : 12);
    new_frame->style = 0;
    new_frame->border = 0;
#ifdef STYLE_COLOR_BORDER
    new_frame->cb_ix = 0;
#else
    new_frame->cb_ix = 0;
#endif
    new_frame->flow = ALIGN_CENTER;
    new_frame->next = new_frame->child = NULL;
    new_frame->box_list = NULL;
    new_frame->leftcount = old_frame->leftcount;
    new_frame->pushcount = old_frame->pushcount;
    new_frame->oldmargin = old_frame->oldmargin;
    PrintBeginFrame(new_frame);
    Push(new_frame);
#endif
}

/*
  FormatElementEnd pops a set of flattened style properties from the
  stack. Note that calls to FormatElementStart and FormatElementEnd must
  always match each other. I.e. your SGML/HTML parser must add any
  implied/omitted tags.
*/

void FormatElementEnd()
{
    Frame *old_frame;
    int element;

    StyleStackElement *stack_el = (StyleStackElement *) HTList_removeLastObject(style_stack);
    element = stack_el->unit.element;
    if (REGISTER_TRACE) 
	fprintf(stderr,"FormatElementEnd:   %d, %d elements left\n", stack_el->unit.element, HTList_count(style_stack));
    
    Free(stack_el->flat);
    Free(stack_el->unit.class);
    Free(stack_el);

    stack_el = (StyleStackElement *) HTList_lastObject (style_stack);
    if (!stack_el) {
	if (REGISTER_TRACE)
	    fprintf(stderr,"stack empty!! \n");
	HTList_destroy(style_stack);
	style_stack = NULL;
	current_flat = NULL;
    } else {
	current_flat = stack_el->flat;
    }

    if(element == TAG_HTML || element == TAG_HTML_SOURCE || 
	(/*element != TAG_TABLE && */
        element != TAG_ABSTRACT &&
	element != TAG_BLOCKQUOTE && element != TAG_CAPTION &&
	element != TAG_NOTE && element != TAG_PRE &&
	element != TAG_QUOTE))
	return;
    if(prepass||damn_table) 
	return;
    old_frame = (Frame *)Pop();
    if(old_frame)
    {
	Frame *parent_frame;
	char *p;
#define PushValue(p, value) ui_n = (unsigned int)value; *p++ = ui_n & 0xFF; *p++ = (ui_n >> 8) & 0xFF

	if(old_frame != (Frame *)1)
	{
	    parent_frame = (Frame *)Pop();
	    if(parent_frame)
		Push(parent_frame);
	    old_frame->length = paintlen - old_frame->info - FRAMESTLEN;
	    old_frame->height = PixOffset - old_frame->offset + 2*StyleGet(S_PADDING);;
	    old_frame->offset -= StyleGet(S_PADDING);
	    PixOffset += StyleGet(S_PADDING);
	    p = paint + old_frame->info + 9;
	    PushValue(p, old_frame->height & 0xFFFF);
	    PushValue(p, (old_frame->height >> 16) & 0xFFFF);
	    PrintFrameLength(old_frame);
	    if(parent_frame == (Frame *)1)  
		PrintEndFrame(NULL, old_frame);
	    else
		PrintEndFrame(parent_frame, old_frame);
	    FreeFrames(old_frame); 
	};
    }; 
}


