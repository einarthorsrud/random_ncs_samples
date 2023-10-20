#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


#define ADDR_LIST_LEN 16384
static bt_addr_le_t addr_list[ADDR_LIST_LEN] = {0};
static uint32_t addr_list_count = 0;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bool new = true;

	for (int i = 0; i < addr_list_count; i++) {
		if (bt_addr_le_cmp(addr, &addr_list[i]) == 0) {
			new = false;
		}
	}

	if (new) {
		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Device found (%u): %s (RSSI %d)\n",
			addr_list_count, addr_str, rssi);


		if (addr_list_count < ADDR_LIST_LEN) {
			/* Add new address to device list. */
			addr_list[addr_list_count] = *addr;
		}

		if (addr_list_count == ADDR_LIST_LEN) {
			printk("** Device list full. There may be duplicates in the log from now on. **\n");
		}

		addr_list_count++;
	}
}

static void start_scan(void)
{
	int err;

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}


int main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	start_scan();
	return 0;
}
