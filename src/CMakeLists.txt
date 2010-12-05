ADD_LIBRARY( common STATIC
	Object.c
	ObjectStore.c
	Transaction.c
	MethodCallObject.c
	ConflictSet.c
	ConflictResolution.c
	Stabilization.c
	Generation.c
	EventQueue.c
	Receiver.c
	Propagate.c
	Replica.c
	Network.c
	DBoid.c
	BDB.c
	Timer.c
	Watch.c
)

LINK_DIRECTORIES(
	"$ENV{HOME}/lib"
	${GLIB_LIBRARY_DIRS}
)

ADD_EXECUTABLE( pride pride.c )

TARGET_LINK_LIBRARIES( pride common ${BDB_LIB_DIR} ${GLIB_LIBRARIES} uuid )
