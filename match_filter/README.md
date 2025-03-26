- generate `.txt` with Matlab
```matlab
input_real = real(x);
input_imag = imag(x);
dlmwrite('input_x_real.txt', input_real', 'precision', '%.10f');
dlmwrite('input_x_imag.txt', input_imag', 'precision', '%.10f');
```
