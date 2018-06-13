/* config.h - configuration for linker */

/* Copyright (C) 1994 Bruce Evans */

/* one of these target processors must be defined */

#undef  I8086			/* Intel 8086 */
#undef  I80386			/* Intel 80386 */
#define MC6809			/* Motorola 6809 */

/* one of these target operating systems must be defined */

#undef  EDOS			/* generate EDOS executable */
#define MINIX			/* generate Minix executable */

/* these may need to be defined to suit the source processor */

#define HOST_8BIT		/* enable some 8-bit optimizations */

/* Any machine can use long offsets but i386 needs them */

/* these must be defined to suit the source libraries */

#define CREAT_PERMS	0666	/* permissions for creat() */
#define EXEC_PERMS	0111	/* extra permissions to set for executable */
