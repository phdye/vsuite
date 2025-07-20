VARCHAR strings need to be manipulated and interoperate with C strings is the various forms:

Vsuite will use short prefixes based on the applicable data types and order of arguments.

For clarity, name the functions after the corresponding Standard C Library function, i.e. vx_strcmp()

<u>**Datatype Indicators**</u>

- 'v' Fixed VARCHAR

- 'x' Pointer to a VARCHAR  
- 'f'  Fixed C String -- A fixed string where sizeof() will return the allocated size of the string.  It could be static or on the stack but may not dynamically allocated.

- 'd' Dynamically allocated C String

- 'p' char * -- a pointer to either a fixed or dynamic C String

<u>**Function / Macro Prefixes**</u>

- 'v_' -- Fixed VARCHAR only, unary or binary routines
- 'x_' -- Pointers to VARCHAR only, unary or binary routines
- 'vx_' -- Binary operations ( Fixed VARCAR , Pointer to VARCHAR )
- 'xv_' -- Binary operations ( Pointer to VARCHAR , Fixed VARCAR )
- 'vf_' -- Binary functions ( Fixed VARCHAR , Fixed C String )
- 'fv_' -- Binary functions ( Fixed C String , Fixed VARCHAR )
- 'vd_' -- Binary functions ( Fixed VARCHAR , Dynamic C String )
- 'dv_' -- Binary functions ( Dynamic C String , Fixed VARCHAR )
- 'vp_' -- Binary functions ( Fixed VARCHAR , Pointer to a C String )
- 'pv_' -- Binary functions ( Pointer to a C String , Fixed VARCHAR )

**<u>Unary Functions</u>** -- for 'v_' and 'x_' prefix

- initialization
- validation
- NUL termination -- validation, application
- duplication
- ltrim, rtrim, trim
- upper, lower
- find character -- i.e. index, rindex
- find substring -- strstr
- any of set - strpbrk
- span of chars, complement of span
- sprintf, vsprint, snprintf, vsnprintf
- safe move, set region, clear content
- *Please recommend more ...*

<u>**Binary Functions**</u>

- string copy, strnpy
- string compare -- case sensitive and case insensitive
- concat
- *Please recommend more ...*