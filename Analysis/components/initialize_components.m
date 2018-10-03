%% Initialization of parameters
% Tx
no_leds_per_TX = 1;
height_tx = 2.8;
% Rx
height_rx = 0.8;

% Communication
modScheme = 'OOK';
N0 = 7.021187833430255e-17;
B = 1e6; % 1MHz transmission bandwidth


%% load the TX topology
tx_topology = load(setup_placement,'no_tx_horz','no_tx_vert','no_tx_horz_total','no_tx_vert_total','pos_tx','id_tx','tx_semiAngle','room_dim');
no_tx_horz = tx_topology.no_tx_horz;
no_tx_vert = tx_topology.no_tx_vert;
no_tx_horz_total = tx_topology.no_tx_horz_total;
no_tx_vert_total = tx_topology.no_tx_vert_total;
no_tx = (tx_topology.no_tx_horz * tx_topology.no_tx_vert);

%% Load the componentes
% room
room = room(1,tx_topology.room_dim,no_tx_horz,no_tx_vert);

% TX 
for i=1:no_tx
    tx(i) = transmitter(tx_topology.id_tx(i),no_leds_per_TX,tx_topology.pos_tx(i,:),[-90,-90],450e-3,tx_topology.tx_semiAngle);
end

% RX
for i=1:no_rx
    rx(i) = receiver(i,[90,0],height_rx,room);
end

% channel 
channel = channel();

% communication parameters
comm = comm(modScheme,no_rx);
comm = comm.setRxNoisePower(N0,B);

% scheduling algorithms
sched = scheduling();