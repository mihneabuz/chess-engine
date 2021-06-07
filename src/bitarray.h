#ifndef _BITARRAY_H_
#define _BITARRAY_H_

///////////////////////////////////////////////////////////
//         Custom implementation of std::bitset          //
//             Only holds 1 byte -> 8 flags              //
//         Has support for arithmetic operations         //
///////////////////////////////////////////////////////////

class bitarray {
  private:
    uint8_t arr;
    
  public:
    bitarray(): arr(0) {};

    inline void operator=(int x) noexcept {
        arr = x;
    }

    // check a bit/flag
    inline bool test(int i) const {
        return (arr & (1 << i));
    }

    // set a bit/flag
    inline void set(int i) {
        arr |= 1 << i;
    }

    // add 1 << i to byte 
    inline void add(int i) {
        arr += 1 << i;
    }

    // get byte
    inline uint8_t to_byte() const {
        return arr;
    }
};


#endif
