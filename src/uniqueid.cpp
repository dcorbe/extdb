//http://stackoverflow.com/questions/2620218/fastest-container-or-algorithm-for-unique-reusable-ids-in-c


#include "uniqueid.h"


IdManager::IdManager()
{
	free_.insert(id_interval(1, std::numeric_limits<int>::max()));
}

int IdManager::AllocateId()
{
	id_interval first = *(free_.begin());
	int free_id = first.left();
	free_.erase(free_.begin());
	if (first.left() + 1 <= first.right()) {
		free_.insert(id_interval(first.left() + 1 , first.right()));
	}
	return free_id;
}

bool IdManager::MarkAsUsed(int id)
{
	id_intervals_t::iterator it = free_.find(id_interval(id,id));
	if (it == free_.end()) {
		return false;
	} else {
		id_interval free_interval = *(it);
		//free_.erase (it);
		if (free_interval.left() < id) {
			free_.insert(id_interval(free_interval.left(), id-1));
			free_.insert(it, id_interval(free_interval.left(), id-1));
		}
		if (id +1 <= free_interval.right() ) {
			free_.insert(id_interval(id+1, free_interval.right()));
			free_.insert(it, id_interval(id+1, free_interval.right()));
		}
		return true;
	}
}

void IdManager::FreeId(int id)
{
	id_intervals_t::iterator it = free_.find(id_interval(id,id));
	if (it != free_.end()  && it->left() <= id && it->right() > id) {
		return ;
	}
	it = free_.upper_bound(id_interval(id,id));
	if (it == free_.end()) {
		return ;
	} else {
		id_interval free_interval = *(it);

		if (id + 1 != free_interval.left()) {
			free_.insert(id_interval(id, id));
		} else {
			if (it != free_.begin()) {
				id_intervals_t::iterator it_2 = it;
				--it_2;
				if (it_2->right() + 1 == id ) {
					id_interval free_interval_2 = *(it_2);
					free_.erase(it);
					free_.erase(it_2);
					free_.insert(
						id_interval(free_interval_2.left(), 
									free_interval.right()));
				} else {
					free_.erase(it);
					free_.insert(id_interval(id, free_interval.right()));
				}
			} else {
					free_.erase(it);
					free_.insert(id_interval(id, free_interval.right()));
			}
		}
	}
}

bool id_interval::operator < (const id_interval& s) const
{
	return 
	  (value_.lower() < s.value_.lower()) && 
	  (value_.upper() < s.value_.lower());
}