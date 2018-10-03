function [] = createFigure(fig_length,fig_width)
scrsz = get(0,'ScreenSize');
figure('Position',[1 scrsz(4)/2 fig_length fig_width]);
end

