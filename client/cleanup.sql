USE trade_data;
delete from `trade_data`.`orders_queue`;
delete from `trade_data`.`pending_orders`;
commit;
