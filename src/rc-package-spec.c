#include <ctype.h>
#include <stdio.h>

/* #define DEBUG 50 */

#include <libredcarpet/rc-package-spec.h>

void
rc_package_spec_init (RCPackageSpec *rcps,
                      gchar *name,
                      guint32 epoch,
                      gchar *version,
                      gchar *release)
{
    g_assert (rcps);

    rcps->name = g_strdup (name);
    rcps->epoch = epoch;
    rcps->version = g_strdup (version);
    rcps->release = g_strdup (release);
} /* rc_package_spec_init */

void
rc_package_spec_copy (RCPackageSpec *old, RCPackageSpec *new)
{
    rc_package_spec_init (new, old->name, old->epoch, old->version,
                          old->release);
}

void
rc_package_spec_free_members (RCPackageSpec *rcps)
{
    g_free (rcps->name);
    g_free (rcps->version);
    g_free (rcps->release);
} /* rc_package_spec_free_members */

gint
rc_package_spec_compare_name (void *a, void *b)
{
    RCPackageSpec *one, *two;

    one = (RCPackageSpec *) a;
    two = (RCPackageSpec *) b;

    return (strcmp (one->name, two->name));
}

static int vercmp(char * a, char * b);

gchar *
rc_package_spec_to_str (RCPackageSpec *spec)
{
    gchar *buf;
    if (spec->epoch) {
        buf = g_strdup_printf ("%s%s%d:%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               spec->epoch,
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    }

    return buf;
}

    
guint rc_package_spec_hash (gconstpointer ptr)
{
    RCPackageSpec *spec = (RCPackageSpec *) ptr;
    guint ret;
    gchar *b = rc_package_spec_to_str (spec);
    ret = g_str_hash (b);
    g_free (b);
    return ret;
}

gint rc_package_spec_compare (gconstpointer ptra, gconstpointer ptrb)
{
    RCPackageSpec *a = (RCPackageSpec *) ptra, *b = (RCPackageSpec *) ptrb;
    gint ret;

#if DEBUG > 40
    fprintf (stderr, "(%s-%d:%s-%s vs. %s-%d:%s-%s)", a->name, a->epoch,
             a->version, a->release, b->name, b->epoch,
             b->version, b->release);
#endif
    if (a->name || b->name) {
        ret = strcmp (a->name ? a->name : "", b->name ? b->name : "");
        if (ret) {
#if DEBUG > 40
            fprintf (stderr, " -> N %s\n", ret > 0 ? ">" : "<");
#endif
            return ret;
        }
    }

    if (a->epoch != b->epoch) {
        ret = a->epoch - b->epoch;
#if DEBUG > 40
        fprintf (stderr, " -> E %s\n", ret > 0 ? ">" : "<");
#endif
        return ret;
    }

    if (a->version || b->version) {
        ret = vercmp (a->version ? a->version : "", b->version ? b->version : "");
        if (ret) {
#if DEBUG > 40
            fprintf (stderr, " -> V %s\n", ret > 0 ? ">" : "<");
#endif
            return ret;
        }
    }

    if (a->release || b->release) {
        ret = vercmp (a->release ? a->release : "", b->release ? b->release : "");
        if (ret) {
#if DEBUG > 40
            fprintf (stderr, " -> R %s\n", ret > 0 ? ">" : "<");
#endif
            return ret;
        }
    }

#if DEBUG > 40
    fprintf (stderr, " -> ==\n");
#endif

    /* If everything passed, then they're equal */
    return 0;
}

gint rc_package_spec_equal (gconstpointer ptra, gconstpointer ptrb)
{
    RCPackageSpec *a = (RCPackageSpec *) ptra, *b = (RCPackageSpec *) ptrb;

    if (a->name && b->name) {
        if (strcmp (a->name, b->name) != 0)
            return FALSE;
    } else if (a->name || b->name) {
        return FALSE;
    }

    if (a->epoch != b->epoch)
        return FALSE;

    if (a->version && b->version) {
        if (strcmp (a->version, b->version) != 0)
            return FALSE;
    } else if (a->version || b->version) {
        return FALSE;
    }

    if (a->release && b->release) {
        if (strcmp (a->release, b->release) != 0)
            return FALSE;
    } else if (a->release || b->release) {
        return FALSE;
    }

    return TRUE;
}

/* This was stolen from RPM */
/* And then slightly hacked on by me */

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
static int
vercmp(char * a, char * b)
{
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;

    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    /* The alloca's were silly. The loop below goes through pains
       to ensure that the old character is restored; this just means
       that we have to make sure that we're not passing in any
       static strings. I think we're OK with that.
    */
    one = str1 = a;
    two = str2 = b;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one && *two) {
	while (*one && !isalnum(*one)) one++;
	while (*two && !isalnum(*two)) two++;

	str1 = one;
	str2 = two;

	/* A number vs. word comparison always goes in favor of the word. Ie:
	   helix1 beats 1. */
	if (!isdigit(*str1) && isdigit(*str2))
		return 1;
	else if (isdigit(*str1) && !isdigit(*str2))
		return -1;

	/* grab first completely alpha or completely numeric segment */
	/* leave one and two pointing to the start of the alpha or numeric */
	/* segment and walk str1 and str2 to end of segment */
	if (isdigit(*str1)) {
	    while (*str1 && isdigit(*str1)) str1++;
	    while (*str2 && isdigit(*str2)) str2++;
	    isnum = 1;
	} else {
	    while (*str1 && isalpha(*str1)) str1++;
	    while (*str2 && isalpha(*str2)) str2++;
	    isnum = 0;
	}

	/* save character at the end of the alpha or numeric segment */
	/* so that they can be restored after the comparison */
	oldch1 = *str1;
	*str1 = '\0';
	oldch2 = *str2;
	*str2 = '\0';

	/* take care of the case where the two version segments are */
	/* different types: one numeric and one alpha */
	if (one == str1) {
            *str1 = oldch1;
            *str2 = oldch2;
            return -1;	/* arbitrary */
        }
	if (two == str2) {
            *str1 = oldch1;
            *str2 = oldch2;
            return -1;
        }

	if (isnum) {
	    /* this used to be done by converting the digit segments */
	    /* to ints using atoi() - it's changed because long  */
	    /* digit segments can overflow an int - this should fix that. */

	    /* throw away any leading zeros - it's a number, right? */
	    while (*one == '0') one++;
	    while (*two == '0') two++;

	    /* whichever number has more digits wins */
	    if (strlen(one) > strlen(two)) {
                *str1 = oldch1;
                *str2 = oldch2;
                return 1;
            }
	    if (strlen(two) > strlen(one)) {
                *str1 = oldch1;
                *str2 = oldch2;
                return -1;
            }
	}

	/* strcmp will return which one is greater - even if the two */
	/* segments are alpha or if they are numeric.  don't return  */
	/* if they are equal because there might be more segments to */
	/* compare */
	rc = strcmp(one, two);
	if (rc) {
            *str1 = oldch1;
            *str2 = oldch2;
            return rc;
        }

	/* restore character that was replaced by null above */
	*str1 = oldch1;
	one = str1;
	*str2 = oldch2;
	two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}
