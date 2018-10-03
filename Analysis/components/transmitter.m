classdef transmitter
    properties (Constant)
        theta = linspace(0,90,180);           
        theta_rad = deg2rad(linspace(0,90,180));
    end
    % assumptions 
    % all LEDS have the same radiation pattern
    % all LEDS are driven by same current I_level
    properties
            id
            no_leds                                 % Number of LEDs per transmitter
            name                                    % Name of LED
            dataset
            semi_angle                              % Semi angle of radiation pattern
            m                                       % Order of lambertian pattern
            WPE                                     % Wall Plug efficiency (conversion electical -> optical power)
            I_bias
            I_max
            max_swing
            V_bias
            dyn_resistance
            n                                       % Ideality factor LED
            Is                                      % Saturation current LED
            Rs                                      % Series resistance of LED
            lumFlux                                 % Total luminous flux in [lumen]
            lumIntensCenter                         % Center luminous intensity in [lumen/sr]
            radPattern_discrete
            radPattern_cont
            pos                                     % Position of transmitter
            angle                                   % angle[1] = Elevation angle of transmitter / angle[2] = Azimuth angle of transmitter
            orient                                  % Orientation unit vector of transmitter          
            efficiency_lens                         % the efficiency of the lens
    end
    
    methods
        %% Constructor
        function obj = transmitter(id,no_leds,pos,angle,I_bias,semi_angle)
            obj.name = 'CREE XT-E';
            obj.dataset = 'CREE_XTE_Tjunction_85deg';
            obj.WPE = 0.4;
            obj.no_leds = no_leds;
            obj.I_bias = I_bias;
            obj.I_max = getMaxCurrent(obj);
            obj.max_swing = obj.I_max;
            obj.efficiency_lens = 0.88; % efficiency of FA10645_TINA-M lens
            obj.lumFlux = getLumFlux_current(obj) * obj.efficiency_lens;
            obj = setSemiAngle_lumFlux(obj,semi_angle); % luminous flux is used to calculate Luminous Center Intensity

            ds = load(obj.dataset);
            obj.n = ds.n;
            obj.Is = ds.Is;
            obj.Rs = ds.Rs;
            obj.dyn_resistance = getDynResistance(obj);

            
            obj.id = id;
            obj.pos = pos;
            obj.angle = angle;
            obj.orient = getOrientation(obj,angle(1),angle(2));
        end
        
        % There are two ways to get the optical power of an LED:
        % 1. luminous center intensity --radiation_pattern--> luminous flux --integral_PSD--> optical power 
        % 2. Voltage and current LED --formula--> electrical power  --WPE-->optical power

        % To get the luminous flux for illumination calculcations:
        % 1. luminous center intensity --radiation_pattern--> luminous flux
        % 2. electrical power --luminous_efficacy--> luminous flux
        
        % for this submission, we used method 2 for both optical power and luminous flux.
        
        %% Get ID of Tx
        function id = getID(obj)
            id = obj.id;
        end
        %% Get number of LEDs per transmitter
        function no_leds = getNoLeds(obj)
            no_leds = obj.no_leds;
        end
        %% Set number of LEDs per transmitter
        function obj = setNoLeds(obj,no_leds)
            obj.no_leds = no_leds;
        end
        %% Determine orientation unit vector of transmitter 
        function orient = getOrientation(obj,angle_elev,angle_azim)
            orient = [cosd(angle_azim)*cosd(angle_elev) sind(angle_azim)*cosd(angle_elev) sind(angle_elev)];
        end
        %% Get position of transmitter
        function position = getPosition(obj)
            position = obj.pos;
        end
        %% Get position of transmitter
        function obj = setPosition(obj,position)
            obj.pos = position;
        end
        
        %% Determine max current resulting in same optical power for illumination and communication with duty cycle 50%
        function I_max = getMaxCurrent(obj)
            ds = load(obj.dataset);
            fit_current_lm = fit(ds.current,ds.luminous_flux,'cubicinterp');
            fit_lm_current = fit(ds.luminous_flux,ds.current,'cubicinterp');
            Poptical_bias = fit_current_lm(obj.I_bias); 
            I_max = fit_lm_current(2*Poptical_bias); % we use 50% duty cycle, so we need 2x optical power of bias for high level
        end

        %% Electrical power via shotkey equation
        function elecPower = getElecPower(obj,I_LED) % only takes into account the bias signal, since the swing is not perceived by the human eye
            V_LED = obj.n * 26e-3 * log(I_LED/obj.Is + 1) + obj.Rs*I_LED;
            elecPower = obj.no_leds * (V_LED .* I_LED);
        end
       
        
        function dyn_resistance = getDynResistance(obj)
            dyn_resistance = (obj.n*25.85e-3/obj.I_bias+2*obj.Rs)/2;
        end
            
        %% Radiation Distribution
        function output = getRadPattern_discrete(obj)
            output = (cos(obj.theta_rad)).^obj.m;
        end
        
        function output = getRadPattern_cont(obj)
            output = fit(obj.theta',getRadPattern_discrete(obj)','gauss7');
        end
        %% Luminous flux       
        function lumFlux = getLumFlux_current(obj)
            ds = load(obj.dataset);
            fit_current_lm = fit(ds.current,ds.luminous_flux,'cubicinterp');
            lumFlux = fit_current_lm(obj.I_bias);
        end
        
        function  lumFlux = getLumFlux_lumIntensCenter(obj)
            func = getRadPattern_discrete(obj) .* sin(obj.theta_rad);
            Omega = 2*pi*trapz(obj.theta_rad,func); % integrate func
            lumFlux = obj.lumIntensCenter*Omega;
        end
        
        %% Luminous intensity
        function lumIntensCenter = getLumIntensCenter(obj)
            lumIntensCenter = obj.lumIntensCenter;
        end
       
        function  lumIntensCenter = getLumIntensCenter_lumFlux(obj)
            func = getRadPattern_discrete(obj) .* sin(obj.theta_rad);
            Omega = 2*pi*trapz(obj.theta_rad,func); % integrate func
            lumIntensCenter = obj.lumFlux/Omega;
        end
        
        %% Semi angle
        function semi_angle = getSemiAngle(obj)
            semi_angle = obj.semi_angle;
        end
        
        function obj = setSemiAngle_lumIntensCenter(obj,semi_angle) % set semi angle based on known luminous intensity center
            obj.semi_angle = semi_angle;
            obj.m = -log(2)/log(cos(obj.semi_angle*pi/180));
            obj.radPattern_discrete = getRadPattern_discrete(obj);
            obj.radPattern_cont = getRadPattern_cont(obj);
            obj.lumFlux = getLumFlux_lumIntensCenter(obj);
        end
        
        function obj = setSemiAngle_lumFlux(obj,semi_angle) % set semi angle based on known luminous flux
            obj.semi_angle = semi_angle;
            obj.m = -log(2)/log(cos(obj.semi_angle*pi/180));
            obj.radPattern_discrete = getRadPattern_discrete(obj);
            obj.radPattern_cont = getRadPattern_cont(obj);
            obj.lumIntensCenter = getLumIntensCenter_lumFlux(obj);
        end
        
        %% Visualization functions
        function visPosTx(obj,output_paper,noillum)
            hold on;
                if(output_paper)
                    if(noillum)
                         scatter(obj.pos(1),obj.pos(2),'filled','MarkerEdgeColor',[0 0 0],...
                        'MarkerFaceColor',[0 0 0],...
                        'LineWidth',5);
                        text(obj.pos(1)-0.10,obj.pos(2)+0.1,['TX',num2str(obj.id)], 'Color', 'black','FontWeight', 'Bold')
                    else
                         scatter(obj.pos(1),obj.pos(2),'filled','MarkerEdgeColor',[1 1 1],...
                        'MarkerFaceColor',[1 1 1],...
                        'LineWidth',5);
                        text(obj.pos(1)-0.10,obj.pos(2)+0.005,['TX',num2str(obj.id)], 'Color', 'black','FontWeight', 'Bold')                        
                    end
                else
                     scatter(obj.pos(1),obj.pos(2),'filled','MarkerEdgeColor',[1 1 1],...
                    'MarkerFaceColor',[1 1 1],...
                    'LineWidth',5);
                    text(obj.pos(1)-0.15,obj.pos(2)+0.005,['TX',num2str(obj.id)], 'Color', 'black','FontWeight', 'Bold')
                end
            hold off;
        end
    end
end