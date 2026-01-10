module Zstd_Accelerator_System #(
    parameter HASH_WIDTH = 32,
    parameter ADDR_WIDTH = 32
) (
    input wire clk,
    rst_n,

    // CPU Command Interface
    input  wire                  cmd_push,
    input  wire [HASH_WIDTH-1:0] cmd_hash,
    input  wire [ADDR_WIDTH-1:0] cmd_offset,
    output wire                  cmd_full,

    // CPU Result Interface
    input  wire                  res_pop,
    output wire [ADDR_WIDTH-1:0] res_match_addr,
    output wire                  res_empty,

    // External DRAM Interface
    output wire [ADDR_WIDTH-1:0] dram_addr,
    output wire                  dram_req,
    input  wire [         511:0] dram_data,
    input  wire                  dram_valid
);

  wire [HASH_WIDTH+ADDR_WIDTH-1:0] q_data;
  wire q_empty, w_ready, w_match, res_q_full;
  wire [ADDR_WIDTH-1:0] w_addr;

  // Request Queue: Stores {Offset, Hash}
  FIFO_Queue #(HASH_WIDTH + ADDR_WIDTH, 16) req_q (
      .clk  (clk),
      .rst_n(rst_n),
      .push (cmd_push),
      .din  ({cmd_offset, cmd_hash}),
      .full (cmd_full),
      .pop  (w_ready && !q_empty && !res_q_full),
      .dout (q_data),
      .empty(q_empty)
  );

  // The Engine: Performs the Multi-way Search
  Tree_Walker_Engine #(HASH_WIDTH, ADDR_WIDTH, 32) walker (
      .clk(clk),
      .rst_n(rst_n),
      .task_valid(!q_empty && !res_q_full),
      .task_hash(q_data[HASH_WIDTH-1:0]),
      .task_offset(q_data[HASH_WIDTH+ADDR_WIDTH-1:HASH_WIDTH]),
      .ready(w_ready),
      .match_found(w_match),
      .best_match_addr(w_addr),
      .dram_addr(dram_addr),
      .dram_req(dram_req),
      .dram_data(dram_data),
      .dram_valid(dram_valid)
  );

  // Result Queue: Stores Match Addresses
  FIFO_Queue #(ADDR_WIDTH, 16) res_q (
      .clk  (clk),
      .rst_n(rst_n),
      .push (w_match),
      .din  (w_addr),
      .full (res_q_full),
      .pop  (res_pop),
      .dout (res_match_addr),
      .empty(res_empty)
  );

endmodule

