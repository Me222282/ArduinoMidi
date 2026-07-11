#[derive(Debug, Clone, Copy, PartialEq)]
pub struct CubicInput
{
    x3: f32,
    x2: f32,
    x: f32
}

impl CubicInput
{
    #[inline]
    #[must_use]
    pub const fn new(x: f32) -> CubicInput
    {
        let x2 = x * x;
        return CubicInput { x3: x2 * x, x2, x };
    }
    #[inline]
    #[must_use]
    pub const fn get_x(self) -> f32
    {
        return self.x;
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Cubic(f32, f32, f32, f32);

impl Cubic
{
    #[inline]
    #[must_use]
    pub const fn compute(self, input: CubicInput) -> f32
    {
        return (self.0 * input.x3) + (self.1 * input.x2) + (self.2 * input.x) + self.3;
    }
    
    /// the points used are:
    /// (`-1.0`, `p1`), (`0.0`, `p2`), (`1.0`, `p3`), (`2.0`, `p4`)
    #[must_use]
    pub const fn generate(p1: u16, p2: u16, p3: u16, p4: u16) -> Cubic
    {
        let d = p2 as f32;
        let b2 = p1 + p3 - p2 - p2;
        let b = (b2 as f32) * 0.5;
        let a6 = p4 + p2 - p3 - p3 - b2;
        let a = (a6 as f32) / 6.0;
        let c = (p3 - p2) as f32 - a - b;
        
        return Cubic(a, b, c, d);
    }
}