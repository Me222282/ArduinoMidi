pub struct Queue<T, const SIZE: usize>
{
    inner: [T; SIZE],
    wr: usize,
    rd: usize,
    len: usize
}

impl<T: Copy, const SIZE: usize> Queue<T, SIZE>
{
    pub fn new(init: T) -> Self
    {
        return Self { inner: [init; SIZE], wr: 0, rd: 0, len: 0 };
    }
    pub fn from_iter(iter: impl Iterator<Item = T>, init: T) -> Self
    {
        let mut inner = [init; SIZE];
        
        let mut count = 0;
        for (s, d) in iter.zip(&mut inner)
        {
            *d = s;
            count += 1;
        }
        
        return Self { inner, wr: count % 5, rd: 0, len: count };
    }
    
    pub fn push(&mut self, value: T)
    {
        if self.len == SIZE { panic!(); }
        self.len += 1;
        
        self.inner[self.wr] = value;
        self.wr = (self.wr + 1) % 5;
    }
    pub fn pull(&mut self) -> Option<T>
    {
        if self.len == 0 { return None; }
        self.len -= 1;
        
        let result = self.inner[self.rd];
        self.rd = (self.rd + 1) % 5;
        return Some(result);
    }
    pub fn clear(&mut self)
    {
        self.wr = 0;
        self.rd = 0;
        self.len = 0;
    }
}