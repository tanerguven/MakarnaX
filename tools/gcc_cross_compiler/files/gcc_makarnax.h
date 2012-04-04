#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()				\
  do {											\
	builtin_define_std ("makarnax");			\
	builtin_define_std ("unix");				\
	builtin_assert ("system=makarnax");			\
	builtin_assert ("system=unix");				\
  } while(0);									\

#undef TARGET_VERSION
#define TARGET_VERSION fprintf(stderr, " (i386 makarnax)");
