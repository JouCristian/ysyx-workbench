module top (
	input [7:0] sw,
	output [7:0] led
);

	decode24 u_decode24 (
		.x (sw[1:0]),
		.en (sw[2]),
		.y (led[3:0])
	);
	
	wire [4:0] unused_sw = sw[7:3];
	assign led[7:4] = 4'b0000;

endmodule
