[y,Fs] = audioread('space_oddity.wav');

fy=fft(y);
L=length(y);
L2=round(L/2);
fa=abs(fy(1:L2)); % half is essential, rest is aliasing
fmax=Fs/2; % maximal frequency
fq=((0:L2-1)/L2)*fmax; % frequencies
plot(fq,fa);

% [nsamples,c]=size(y);
% fmax=Fs;
% 
% N=nsamples;
% % fscale=linspace(0,fmax/2,floor(N/2));
% spectrum=fft(y,N);
% 
% %plot the magnitude
% % plot(fscale, 20*log10(abs(spectrum(1:length(fscale)))));
% plot(spectrum);
% % axis([0 6000 -80 80]);
% xlabel('Frequency (Hz)');
% % ylabel('Module(DB)');