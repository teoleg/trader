use trade_data;
drop function place_order_fn;
create function place_order_fn returns integer soname 'libporder.so';
