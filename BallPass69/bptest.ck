Impulse i => BallPass69 bp => dac;

200. => bp.order;
0.7 => bp.g;

0.8 => i.next;


repeat(400) {
    1::samp => now;
    <<<bp.last()>>>;
}