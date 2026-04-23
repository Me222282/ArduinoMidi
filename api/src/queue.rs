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
    
    pub fn push(&mut self, value: T)
    {
        if self.len == SIZE { panic!(); }
        self.len += 1;
        
        self.inner[self.wr] = value;
        self.wr += 1;
    }
    pub fn pull(&mut self) -> Option<T>
    {
        if self.len == 0 { return None; }
        self.len -= 1;
        
        let result = self.inner[self.rd];
        self.rd += 1;
        return Some(result);
    }
    pub fn clear(&mut self)
    {
        self.wr = 0;
        self.rd = 0;
        self.len = 0;
    }
}