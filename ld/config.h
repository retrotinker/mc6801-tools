/* config.h - configuration for linker */

/* Copyright (C) 1994 Bruce Evans */

/* these may need to be defined to suit the source processor */

#define HOST_8BIT		/* enable some 8-bit optimizations */

/* Any machine can use long offsets but i386 needs them */

/* these must be defined to suit the source libraries */

#define CREAT_PERMS	0666	/* permissions for creat() */
#define EXEC_PERMS	0111	/* extra permissions to set for executable */
