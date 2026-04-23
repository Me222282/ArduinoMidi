use core::ops::{Index, IndexMut};

pub struct RA<T, const CAP: usize>
{
    inner: [T; CAP],
    len: usize
}

impl<T, const CAP: usize> AsRef<[T]> for RA<T, CAP>
{
    fn as_ref(&self) -> &[T]
    {
        return &self.inner[0..self.len];
    }
}
impl<T, const CAP: usize> AsMut<[T]> for RA<T, CAP>
{
    fn as_mut(&mut self) -> &mut [T]
    {
        return &mut self.inner[0..self.len];
    }
}

impl<T, const CAP: usize> IndexMut<usize> for RA<T, CAP>
{
    fn index_mut(&mut self, index: usize) -> &mut Self::Output
    {
        return self.as_mut().index_mut(index);
    }
}

impl<T, const CAP: usize> Index<usize> for RA<T, CAP>
{
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output
    {
        return self.as_ref().index(index);
    }
}

impl<T, const CAP: usize> RA<T, CAP>
    where [u8; CAP * core::mem::size_of::<T>()]: Sized
{
    pub fn from_iter(iter: impl Iterator<Item = T>) -> Self
    {
        let init = [0u8; CAP * core::mem::size_of::<T>()];
        let mut inner = unsafe { core::mem::transmute_copy::<_, [T; CAP]>(&init) };
        
        let mut count = 0;
        for (s, d) in iter.zip(&mut inner)
        {
            *d = s;
            count += 1;
        }
        
        return Self { inner, len: count };
    }
    
    #[inline]
    pub fn len(&self) -> usize
    {
        return self.len;
    }
}

impl<T: Copy, const CAP: usize> RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    pub fn new(value: T, size: usize) -> Self
    {
        return Self { inner: [value; CAP], len: size };
    }
}

impl<T: Default, const CAP: usize> Default for RA<T, CAP>
    where [(); CAP * core::mem::size_of::<T>()]: Sized
{
    fn default() -> Self
    {
        let init = [0u8; CAP * core::mem::size_of::<T>()];
        let inner = unsafe { core::mem::transmute_copy::<_, [T; CAP]>(&init) };
        return Self { inner, len: 0 };
    }
}
