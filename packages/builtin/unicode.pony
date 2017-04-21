primitive Unicode
  fun max_rune(): U32 =>
    """
    Return maximum valid Unicode code point.
    """
    0x0010FFFF
  
  fun replacement_character(): U32 =>
    """
    Return Unicode Replacement Character (U+FFFD) (�).
    """
    0xFFFD

  fun max_ascii(): U32 =>
    """
    Return maximum ASCII value. 
    """
    0x007F

  fun max_latin1(): U32 =>
    """
    Return maximum Latin-1 value.
    """
    0x00FF

primitive UTF8
  fun encode(value: U32): (USize, U8, U8, U8, U8) =>
    if value < 0x80 then
      (1, value.u8(), 0, 0, 0)
    elseif value < 0x800 then
      (
        2,
        ((value >> 6) or 0xC0).u8(),
        ((value and 0x3F) or 0x80).u8(),
        0,
        0
      )
    elseif value < 0xD800 then
      (
        3,
        ((value >> 12) or 0xE0).u8(),
        (((value >> 6) and 0x3F) or 0x80).u8(),
        ((value and 0x3F) or 0x80).u8(),
        0
      )
    elseif value < 0xE000 then
      // UTF-16 surrogate pairs are not allowed.
      (3, 0xEF, 0xBF, 0xBD, 0)
    elseif value < 0x10000 then
      (
        3,
        ((value >> 12) or 0xE0).u8(),
        (((value >> 6) and 0x3F) or 0x80).u8(),
        ((value and 0x3F) or 0x80).u8(),
        0
      )
    elseif value < 0x110000 then
      (
        4,
        ((value >> 18) or 0xF0).u8(),
        (((value >> 12) and 0x3F) or 0x80).u8(),
        (((value >> 6) and 0x3F) or 0x80).u8(),
        ((value and 0x3F) or 0x80).u8()
      )
    else
      // Code points beyond 0x10FFFF are not allowed.
      (3, 0xEF, 0xBF, 0xBD, 0)
    end

// TODO alias String as UTF8String

class val UTF8Str
  //is (Seq[U32] & Comparable[Str[E] box] & Stringable)
  """
  An ordered collection of bytes viewed through an encoding.
  """
  var _size: USize
  var _alloc: USize
  var _ptr: Pointer[U8]

  new create(len: USize = 0) =>
    """
    An empty string. Enough space for len bytes is reserved.
    """
    _size = 0
    _alloc = len.min(len.max_value() - 1)
    _ptr = Pointer[U8]._alloc(_alloc)

  fun size(): USize =>
    _size

  fun val array(): Array[U8] val =>
    """
    Returns an Array[U8] that that reuses the underlying data pointer.
    """
    recover
      Array[U8].from_cpointer(_ptr._unsafe(), _size, _alloc)
    end

  fun ref reserve(len: USize) =>
    """
    Reserve space for len bytes.
    """
    if _alloc <= len then
      let max = len.max_value() - 1
      let min_alloc = len.min(max)
      if min_alloc <= (max / 2) then
        _alloc = min_alloc.next_pow2()
      else
        _alloc = min_alloc.min(max)
      end
      _ptr = _ptr._realloc(_alloc)
    end

  fun ref push(value: U32) =>
    """
    Push a UTF-32 code point.
    """
    let encoded = UTF8.encode(value)
    let i = _size
    _size = _size + encoded._1
    reserve(_size)
    _set(i, encoded._2)
    if encoded._1 > 1 then
      _set(i + 1, encoded._3)
      if encoded._1 > 2 then
        _set(i + 2, encoded._4)
        if encoded._1 > 3 then
          _set(i + 3, encoded._5)
        end
      end
    end

  fun ref _set(i: USize, value: U8): U8 =>
    """
    Unsafe update, used internally.
    """
    _ptr._update(i, value)
