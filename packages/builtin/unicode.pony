
primitive UTF8
  fun max_rune(): U32 =>
    """
    Maximum valid Unicode code point.
    """
    0x0010FFFF

  fun max_bytes(): U8 =>
    """
    Maximum number of bytes of a UTF-8 encoded Unicode character.
    """
    4
  
  fun replacement_character(): U32 =>
    """
    Unicode Replacement Character (U+FFFD) (�).
    """
    0xFFFD

  fun max_ascii(): U32 =>
    """
    Maximum ASCII value. 
    """
    0x007F

  fun max_latin1(): U32 =>
    """
    Maximum Latin-1 value.
    """
    0x00FF

  fun width(r: U32): USize =>
    """
    Return the number of bytes the given code point would require if encoded.
    """
    if r < (1 << 7) then 1
    elseif r < (1 << 11) then 2
    elseif r < (1 << 16) then 3
    else 4
    end

  fun encode(r: U32): (USize, U8, U8, U8, U8) =>
    """
    Encode the code point into UTF-8. Returns a tuple with the size of the
    encoded data and then four bytes of data.
    """
    if r < (1 << 7) then
      (1, r.u8(), 0, 0, 0)
    elseif r < (1 << 11) then
      ( 2
      , (0xC0 or (r >> 6)).u8()
      , (0x80 or (r and 0x3F)).u8()
      , 0
      , 0
      )
    elseif r < 0xD800 then
      ( 3
      , (0xE0 or (r >> 12)).u8()
      , (0x80 or ((r >> 6) and 0x3F)).u8()
      , (0x80 or (r and 0x3F)).u8()
      , 0
      )
    elseif r <= 0xDFFF then
      // UTF-16 surrogate pairs are not allowed.
      (3, 0xEF, 0xBF, 0xBD, 0)
    elseif r < 0x10000 then
      ( 3
      , (0xE0 or (r >> 12)).u8()
      , (0x80 or ((r >> 6) and 0x3F)).u8()
      , (0x80 or (r and 0x3F)).u8()
      , 0
      )
    elseif r < 0x110000 then
      ( 4
      , (0xF0 or (r >> 18)).u8()
      , (0x80 or ((r >> 12) and 0x3F)).u8()
      , (0x80 or ((r >> 6) and 0x3F)).u8()
      , (0x80 or (r and 0x3F)).u8()
      )
    else
      // Code points beyond 0x10FFFF are not allowed.
      (3, 0xEF, 0xBF, 0xBD, 0) // UTF-8 Replacement Character
    end

  fun decode(bs: ReadSeq[U8]): (U32, USize) ? =>
    """
    Return a pair of the first UTF-8 encoded rune from the byte array and its
    width in bytes.
    """
    let b0 = bs(0)
    if b0 < 0x80 then (b0.u32_unsafe(), 1) // ASCII
    elseif b0 < 0xC0 then error
    else
      let w: USize =
        if b0 < 0xE0 then 2
        elseif b0 <= 0xF0 then 3
        else 4
        end

      (let lo: U8, let hi: U8) = 
        match b0 >> 4
        | 0 => (0x80, 0xBF)
        | 1 => (0xA0, 0xBF)
        | 2 => (0x80, 0x9F)
        | 3 => (0x90, 0xBF)
        else (0x80, 0x8F)
        end

      let mx: U32 = 0x3F
      let m2: U32 = 0x1F
      let m3: U32 = 0x0F
      let m4: U32 = 0x07
      let locb: U8 = 0x80
      let hicb: U8 = 0xBF

      let b1 = bs(1)
      if (b1 < lo) or (hi < b1) then error end
      if w == 2 then
        let r = 
          ((b0.u32_unsafe() and m2) << 6) or
          (b1.u32_unsafe() and mx)
        return (r, 2)
      end
      
      let b2 = bs(2)
      if (b2 < locb) or (hicb < b2) then error end
      if w == 3 then
        let r =
          ((b0.u32_unsafe() and m3) << 12) or
          ((b1.u32_unsafe() and mx) << 6) or
          (b2.u32_unsafe() and mx)
        return (r, 3)
      end

      let b3 = bs(3)
      if (b3 < locb) or (hicb < b3) then error end
      let r =
        ((b0.u32_unsafe() and m4) << 18) or
        ((b1.u32_unsafe() and mx) << 12) or
        ((b2.u32_unsafe() and mx) << 6) or
        (b3.u32_unsafe() and mx)
      (r, 4)
    end
