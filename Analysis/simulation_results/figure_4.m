%%  -- Code to reproduce figure 4 -- 
clear all;
close all;

%% Initialize parameters for LED CREE XT-E
obj.n = 2.677857214852;
obj.Is = 1.144374946827e-018;
obj.Rs = 0.1927031715938;
obj.I_bias = 450e-3;

%% perform the required calculations
obj.I = linspace(0,2*obj.I_bias,100);

% model of LED power consumption + first derivative + second derivative
P_LED_approx_illum = obj.n * 26e-3 * log(obj.I_bias/obj.Is + 1)*obj.I_bias + obj.Rs*obj.I_bias^2; 
P_LED_approx_illum_deriv = obj.n*26e-3*(log((obj.I_bias/obj.Is)+1)+ (obj.I_bias/(obj.I_bias+obj.Is)))+2*obj.Rs*obj.I_bias;
P_LED_approx_illum_deriv2 = (obj.n*26e-3/obj.I_bias)+2*obj.Rs;


swing = linspace(0,2*obj.I_bias,100);
for i=1:length(swing)
    I_low = obj.I_bias - swing(i)/2;
    I_high = obj.I_bias + swing(i)/2;
    P_LED_exact_low = obj.n * 26e-3 * log((I_low/obj.Is) + 1)*I_low + obj.Rs*I_low^2;
    P_LED_exact_high = obj.n * 26e-3 * log((I_high/obj.Is) + 1)*I_high + obj.Rs*I_high^2;
    P_LED_exact_avg(i) = 0.5*P_LED_exact_low+ 0.5*P_LED_exact_high;

    P_LED_approx_low = P_LED_approx_illum + P_LED_approx_illum_deriv *(I_low-obj.I_bias) + P_LED_approx_illum_deriv2/2*(I_low-obj.I_bias)^2;
    P_LED_approx_high = P_LED_approx_illum + P_LED_approx_illum_deriv *(I_high-obj.I_bias) + P_LED_approx_illum_deriv2/2*(I_high-obj.I_bias)^2;
    P_LED_approx_avg(i) = 0.5*P_LED_approx_low+ 0.5*P_LED_approx_high; 
end

% compute the difference in percentage between the exact power consumption
% and the approximated power consumption based on the Taylor expansion
error = abs(P_LED_approx_avg - P_LED_exact_avg)./P_LED_exact_avg*100; 


%% plot the result
figure;
plot(swing*1000,error);
xlabel('The swing level [mA]')
ylabel('Relative error [%]');
grid on;
axis([0 1000 0 1.5])
