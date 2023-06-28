
extern	int		numthreads;

void ThreadSetDefault (void);
int	GetThreadWork (void);
void RunThreadsOnIndividual (int workcnt, qboolean showpacifier, void(*func)(int));
void RunThreadsOn (int workcnt, qboolean showpacifier, void(*func)(int));
void ThreadLock (void);
void ThreadUnlock (void);

