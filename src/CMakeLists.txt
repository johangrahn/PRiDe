ADD_LIBRARY( common STATIC
#	Object.c
#	OptimisticDb.c
#	Transaction.c
	MethodCallObject.c
	ConflictSet.c
#	ConflictResolution.c
#	Stabilization.c
	Generation.c
	EventQueue.c
	Receiver.c
#	Network.c
#	Replica.c
#	ReplicaList.c
	Propagate.c
	Replica.c
	Network.c
)

link_directories(
	${GLIB_LIBRARY_DIRS}
)

add_executable( pride pride.c )

target_link_libraries( pride common ${BDB_LIB_DIR} ${GLIB_LIBRARIES} )
