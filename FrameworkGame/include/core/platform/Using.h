#if !defined(USING)
#define IN_USE &&
#define NOT_IN_USE &&!
#define USE_IF(x) &&((x)?1:0)&&
#define USING(x) (1 x 1)
#endif // #if !defined(USING)