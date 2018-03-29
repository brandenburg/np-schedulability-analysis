#ifndef CONFIG_H
#define CONFIG_H

// DM : debug message -- disable for now
// #define DM(x) std::cerr << x
#define DM(x)

// #define CONFIG_COLLECT_SCHEDULE_GRAPH

#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
#define CONFIG_PARALLEL
#endif

#ifndef NDEBUG
#define TBB_USE_DEBUG 1
#endif

#endif
