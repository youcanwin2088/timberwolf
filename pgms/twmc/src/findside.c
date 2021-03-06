/* ----------------------------------------------------------------- 
FILE:	    findside.c
DESCRIPTION:This file contains the routines to find what side the
	    pins are located.
CONTENTS:   INT findside( pSideArray, cellptr , x , y )
		PSIDEBOX  *pSideArray ;
		CELLBOXPTR cellptr ;
		INT x , y ;
	    loadside( pSideArray, side , factor )
		PSIDEBOX  *pSideArray ;
		INT side ;
		DOUBLE factor ;
DATE:	    Aug  7, 1988 
REVISIONS:  Apr 23, 1990 - added error checking for soft pins and
		add them correctly to the PSIDE array.
	    Thu Jan 17 00:51:01 PST 1991 - fixed bug in the case
		of T shaped cells. Now finds correct side.
	    Wed Feb 13 23:35:07 EST 1991 - renamed set routines.
----------------------------------------------------------------- */
#ifndef lint
static char SccsId[] = "@(#) findside.c version 3.5 2/13/91" ;
#endif

#include <custom.h>
#include <initialize.h>
#include <yalecad/debug.h>
#include <yalecad/set.h>

#define HOWMANY 0

/* returns which side a pin is located */
INT findside( pSideArray, cellptr , x , y )
PSIDEBOX  *pSideArray ;
CELLBOXPTR cellptr ;
INT x , y ;
{

INT k , min , kmin, dist, begin, end ;
INT xc, yc, b, e ;
INT *yvert, *xvert ;

min = INT_MAX ;
kmin = 1 ; /* default side */
xc = cellptr->xcenter ;
yc = cellptr->ycenter ;
xvert = cellptr->vertices->x ;
yvert = cellptr->vertices->y ;
for( k = 1 ; k <= cellptr->numsides ; k++ ) {
    if( pSideArray[k].vertical == 1 ) {
	dist = ABS(x - pSideArray[k].position) ;
	b = yvert[k] + yc ;
	if( k == cellptr->numsides ){
	    e = yvert[1] + yc ;
	} else {
	    e = yvert[k+1] + yc ;
	}
	begin = MIN( b, e ) ;
	end   = MAX( b, e ) ;

	if( begin <= y && y <= end && dist < min ){ 
	    min = ABS(x - pSideArray[k].position) ;
	    kmin = k ;
	}
    } else {
	dist = ABS(y - pSideArray[k].position) ;
	b = xvert[k] + xc ;
	if( k == cellptr->numsides ){
	    e = xvert[1] + xc ;
	} else {
	    e = xvert[k+1] + xc ;
	}
	begin = MIN( b, e ) ;
	end   = MAX( b, e ) ;

	if( begin <= x && x <= end && dist < min ){ 
	    min = ABS(y - pSideArray[k].position) ;
	    kmin = k ;
	}
    }
}
return( kmin ) ;

} /* end findside */


/* load the side with a pin count and add factor */
loadside( pSideArray, side , factor )
PSIDEBOX  *pSideArray ;
INT side ;
DOUBLE factor ;
{

pSideArray[side].pincount += factor ;

return ;
} /* end loadside */


load_soft_pins( ptr, pSideArray )
CELLBOXPTR ptr ;
PSIDEBOX *pSideArray ;
{

    INT i ;                        /* counter */
    INT j ;                        /* counter */
    INT howmany ;                  /* number of softpins for cell */
    INT restrict ;                 /* counter */
    INT match ;                    /* count number of matching restricts*/
    INT numsides ;                 /* number of side restrictions */
    INT numrestricts ;             /* current # restricts for child */
    INT parent_restricts ;         /* current # restricts for parent */
    BOOL abortPin ;                /* abort load if true */
    PINBOXPTR pin ;                /* current pin */
    PINBOXPTR parent ;             /* parent to current pin */
    SOFTBOXPTR spin ;              /* soft information for current pin */
    YSETPTR side_set ;             /* set of valid sides for pin */
    YSETLISTPTR curside ;          /* used to enumerate the set */

    if(!(ptr->softpins)){
	return ;
    }

    side_set = Yset_init( 1, ptr->numsides ) ;
    howmany = (INT) ptr->softpins[HOWMANY] ;
    for( i = 1 ; i <= howmany; i++ ){

	/**** DETERMINE THE VALID SIDES FOR A SOFTPIN ****/
	pin = ptr->softpins[i] ;
	spin = pin->softinfo ;

	if( pin->type == HARDPINTYPE || pin->type == PINGROUPTYPE ){
	    continue ;
	}

	/* now remove restrictions if present */
	numrestricts = spin->restrict[HOWMANY] ;

	/* initialize set to the empty set */
	Yset_empty( side_set ) ;

	if( numrestricts == 0 ){
	    /* everybody is valid */
	    for( j = 1; j <= ptr->numsides; j++ ){
		Yset_add( side_set, j ) ;
	    }
	} else {
	    /* just validate the given list of sides */
	    for( restrict = 1; restrict <= numrestricts; restrict++ ){
		Yset_add( side_set, spin->restrict[restrict] ) ;
	    }
	}
	/* at this point we have checked the leaf, check parents for */
	/* consistency if pin is not a softequiv type */
	parent = spin->parent ;
	abortPin = FALSE ;
	while( parent && pin->type != SOFTEQUIVTYPE ){
	    spin = parent->softinfo ;
	    parent_restricts = spin->restrict[HOWMANY] ;

	    /* check the various cases of number of restrictions */
	    if( numrestricts != 0 && parent_restricts == 0 ){
		/* we have an inconsistency */
		sprintf( YmsgG, "Pin:%s has inconsistent restrictions\n",
		    pin->pinname ) ;
		M( ERRMSG, "load_soft_pins", YmsgG ) ;
		abortPin = TRUE ;
	    } else if( numrestricts != 0 && parent_restricts != 0 ){
		/* need to make sure that parent is more restrictive */
		/* than child */
		match = 0 ; /* initially no matching restrictions */
		for( restrict = 1;restrict<=parent_restricts;restrict++ ){
		    if( Yset_member( side_set, spin->restrict[restrict] ) ){
			match++ ;
		    } else {
		        Yset_add( side_set, spin->restrict[restrict] ) ;
		    }
		}
		if( match != numrestricts ){
		    sprintf( YmsgG, 
			"Pin:%s has inconsistent restrictions\n",
			pin->pinname ) ;
		    M( ERRMSG, "load_soft_pins", YmsgG ) ;
		    abortPin = TRUE ;
		}
	    } else if( numrestricts == 0 && parent_restricts != 0 ){
		Yset_comp( side_set ) ;
		for( restrict = 1;restrict<=parent_restricts;restrict++ ){
		    Yset_add( side_set, spin->restrict[restrict] ) ;
		}
	    } /* else if numrestricts == 0 && parent_restricts == 0 ) */
	    /* don't have to do anything for this case */

	    /* now prepare for possible next level of hierarchy */
	    numrestricts = parent_restricts ;
	    parent = spin->parent ;
	}

	if( abortPin ){
	    continue ;
	}

	/* now enumerate the set of valid sides for the pin */
	numsides = Yset_size( side_set ) ;
	if( numsides == 0 ){
	    sprintf( YmsgG, "Pin:%s has no valid side\n", pin->pinname ) ;
	    M( ERRMSG, "load_soft_pins", YmsgG ) ;
	    continue ;
	}

	for(curside=Yset_enumerate(side_set);curside;curside=curside->next ){
	    loadside( pSideArray, curside->node, 1.0 / numsides ) ;
	}
    }
    Yset_free( side_set ) ;
} /* end load_soft_pins */
