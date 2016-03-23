#include "ringbuffer.h"

size_t rb_get_write_space(RingBuffer *rb){
	size_t size = 0;
	int i = 0;
	for(i=0; i < rb->nb_readers; i++){
		int rsize = (rb->m_readers[i]+rb->m_size-rb->m_writer-1)%rb->m_size;
		if(i==0 || rsize < size)
			size = rsize;
	}
	return size;
}

size_t rb_get_read_space(RingBuffer *rb, int r){
	if(r >= rb->nb_readers)
		return -1;
	else{
		if(rb->m_writer > rb->m_readers[r])
			return rb->m_writer - rb->m_readers[r];
		else
			return (rb->m_writer + rb->m_size-rb->m_readers[r])%rb->m_size;
	}
}

size_t rb_get_max_read_space(RingBuffer* rb){
	size_t ret=0, tmp;
	int i;
	for(i=0;i<rb->nb_readers;i++){
		tmp=rb_get_read_space(rb, i);
		if(tmp > ret)
			ret=tmp;
	}
	return ret;
}

size_t rb_read(RingBuffer *rb, uint8_t* target, int r, size_t nb){
	size_t available = rb_get_read_space(rb, r);
	if(available <= 0)
		return 0;
	else{
		if(nb > available){
			//std::cout << "Warning: trying to read " << nb << " bytes from ring buffer but only " << available << " available" << std::endl;
			nb = available;
		}
		size_t to_read = nb;
		if(rb->m_readers[r] + nb >= rb->m_size){ // we will have to go back to the beginning 
			to_read = rb->m_size-rb->m_readers[r]; // number of bytes we can read before looping
		}
		memcpy(target, rb->m_buffer+rb->m_readers[r], to_read);
		if(to_read < nb){
			memcpy(target+to_read, rb->m_buffer , nb-to_read);
		}
		rb->m_readers[r] = (rb->m_readers[r] + nb)%rb->m_size;
		return nb;
	}
}

size_t rb_skip(RingBuffer* rb, size_t to_skip, int r){
	size_t available = rb_get_read_space(rb, r);
	if(available <= 0)
		return 0;
	else{
		if(to_skip > available){
			//std::cout << "Warning: trying to read " << nb << " bytes from ring buffer but only " << available << " available" << std::endl;
			to_skip = available;
		}
		rb->m_readers[r] = (rb->m_readers[r] + to_skip)%rb->m_size;
		return to_skip;
	}
}

void rb_reset(RingBuffer* rb){
	rb->m_writer=0;
	int i=0;
	for(i=0;i<rb->nb_readers;i++){
		rb->m_readers[i]=0;
	}
}

size_t rb_read_overlap(RingBuffer *rb, uint8_t* target, int r, size_t nb, float overlap){
	size_t available = rb_get_read_space(rb, r);
	int to_increment=(int) nb*overlap;
	if(available <= 0)
		return 0;
	else{
		if(nb > available){
			//std::cout << "Warning: trying to read " << nb << " bytes from ring buffer but only " << available << " available" << std::endl;
			nb = available;
		}
		size_t to_read = nb;
		if(rb->m_readers[r] + nb >= rb->m_size){ // we will have to go back to the beginning 
			to_read = rb->m_size-rb->m_readers[r]; // number of bytes we can read before looping
		}
		memcpy(target, rb->m_buffer+rb->m_readers[r], to_read);
		if(to_read < nb){
			memcpy(target+to_read, rb->m_buffer , nb-to_read);
		}
		if(to_increment > nb)
			to_increment=nb;
		rb->m_readers[r] = (rb->m_readers[r] + to_increment)%rb->m_size;
		//std::cout << "position is now: " << rb->m_readers[r] << "\n";
		return nb;
	}
}

size_t rb_write(RingBuffer* rb, const uint8_t* source, size_t nb){
	size_t written=0;
	if(nb==0)
		return 0;
	size_t available = rb_get_write_space(rb);
	if(nb > available){
		//std::cout << "Warning: trying to write " << nb << " bytes from ring buffer but only " << available << " available" << std::endl;	
		nb = available;
	}
	size_t to_write = nb;
	if(rb->m_writer + nb >= rb->m_size){ // we will have to go back to the beginning 
		to_write = rb->m_size-rb->m_writer; // number of bytes we can read before looping
	}
	memcpy(rb->m_buffer+rb->m_writer, source, to_write);
	if(to_write < nb){
		memcpy(rb->m_buffer, source+to_write, nb-to_write);
	}
	rb->m_writer = (rb->m_writer + nb)%rb->m_size;
	return nb;
}

size_t rb_zero(RingBuffer* rb, size_t nb){
	size_t written=0;
	if(nb==0)
		return 0;
	size_t available = rb_get_write_space(rb);
	if(nb > available){
		std::cout << "Warning: trying to write " << nb << " bytes from ring buffer but only " << available << " available" << std::endl;	
		nb = available;
	}
	size_t to_write = nb;
	if(rb->m_writer + nb > rb->m_size){ // we will have to go back to the beginning 
		to_write = rb->m_size-rb->m_writer; // number of bytes we can read before looping
	}
	memset(rb->m_buffer+rb->m_writer, 0, to_write);
	if(to_write < nb){
		memset(rb->m_buffer, 0, nb-to_write);
	}
	rb->m_writer = (rb->m_writer + nb)%rb->m_size;
	return nb;
}


int rb_create(RingBuffer* rb, size_t length, int r){
	rb->m_buffer = (uint8_t*) malloc((length+1)*sizeof(uint8_t));
	if(!(rb->m_buffer)){
		std::cerr << "Error: on " << length*sizeof(size_t) << " bytes allocation in rb_create" << std::endl;
		return -1;
	}
	rb->m_mlocked = false;
    rb->m_writer=0;
    rb->m_readers = (size_t *) malloc(r*sizeof(size_t));
    if(!(rb->m_readers)){
    	std::cerr << "Error: on " << r*sizeof(size_t) << " bytes allocation in rb_create" << std::endl;
		return -1;
    }
    rb->m_size = length+1;
    rb->nb_readers = r;
    for(int i=0; i < r; i++){
    	rb->m_readers[i]=0;
    }
    return 0;
}

void print_buffer_stats(RingBuffer* rb){
	std::cout << "writer at pos: " << rb->m_writer << std::endl;
	for(int i=0;i<rb->nb_readers;i++){
		std::cout << "reader " << i << " at pos: " << rb->m_readers[i] << std::endl;
	}

}

void rb_free(RingBuffer* rb){
	free(rb->m_buffer);
	free(rb->m_readers);
	free(rb);
}

int add_reader(RingBuffer* rb){
	int n = rb->nb_readers;
	rb->m_readers = (size_t *) realloc(rb->m_readers, (n+1)*sizeof(size_t));
	if(!rb->m_readers)
		return -1;
	rb->nb_readers+=1;
	rb->m_readers[n]=0;
	return n;
}