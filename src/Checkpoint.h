#ifndef INCLUDED_CHECKPOINT
#define INCLUDED_CHECKPOINT

template< typename T >
class Checkpoint {
	T& x;
	T old;
	bool committed = false;
public:
	void commit() { committed = true; }
	Checkpoint(T& x) : x(x), old(x) {}
	~Checkpoint() {
		if (!committed) x = old;
	}
};

#endif //ndef INCLUDED_CHECKPOINT
