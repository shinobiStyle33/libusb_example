/*
 * This is example libusb code.
 * 
 *
 * Author: Scottie <shinobi.style3@gmail.com>
 */

#include <stdio.h>
#include <libusb-1.0/libusb.h>

/* You may need to change the VENDOR_ID and PRODUCT_ID depending on your device.
 */
#define VENDOR_ID 0xffff
#define PRODUCT_ID 0xffff

#define	IF_NUM 1
#define	UNLIMITED_TIMEOUT 0
#define	CHECK_ERR(rc)	        if (rc < 0) { goto out; }

// We use a global variable to keep the device handle
static struct libusb_device_handle *devh = NULL;
static unsigned char buf[0x100];

/*
 * init_usblib()
 *  function:   initialize lib-usb
 *  args:       none
 *  return:     success / fail (<0)
 */
int init_usblib(void)
{
	int		rc;
	/* initialize lsusb */
    rc = libusb_init(NULL);
    if (rc < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
		return rc;
    }

    /* Set debugging output to max level. */
    libusb_set_debug(NULL, 3); 

	return rc;
}

/*
 * get_device_handle(vid, pid)
 *  function:   
 *  args:       vid:    vendor id
 *              pid:    product id
 *  return:     device handle / NULL
 */
libusb_device_handle *get_device_handle(uint16_t vid, uint16_t pid)
{
    libusb_device_handle    *dh = NULL;
    int                     if_num;
    int                     rc;

    while (dh == NULL) {
		dh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	}

    for (if_num = 0; if_num < IF_NUM; if_num++) {
        if (libusb_kernel_driver_active(dh, if_num)) {
            libusb_detach_kernel_driver(dh, if_num);
            fprintf(stdout,"detach kernrl driver\n");
            fprintf(stdout, "interface number is %d\n",if_num);
        }
        rc = libusb_claim_interface(dh, if_num);
        if (rc < 0) {
            fprintf(stderr, "Error claiming interface: %s\n",
                    libusb_error_name(rc));
            return NULL;
        }
    }

    return dh;
}


int reattach_kernel_driver(libusb_device_handle *dh)
{
	int	if_num;
	int	rc;

	for (if_num = IF_NUM-1; if_num >= 0; if_num--) {
		rc = libusb_release_interface(devh, if_num);
		if (rc < 0) {
			fprintf(stderr, "libusb_release_interface() failed. (%d), if=%d\n", rc, if_num);
			return rc;
		}
		rc = libusb_attach_kernel_driver(dh, if_num);
		fprintf(stdout, "re-attach kernel driver (if=%d)\n", if_num);
		if (rc < 0) {
			fprintf(stderr, "re-attach kernel driver failed. (%d)\n", rc);
//				return rc;
		}
//		}
	}

	return 0;
}

/*
 * tx_control(type, req, wVal, wIdx, data, wLen)
 *  function:   send control data (send request, and send data)
 *  args:       type:   bmRequestType
 *              req:    bRequestCode
 *              wVal:   wValue
 *              wIdx:   wIndex
 *              data:   data to to be transmitted
 *              wLen:   wLength
 *  return:     success(0) / fail (<0)
 */
int tx_control(uint8_t type, uint8_t req , uint16_t wVal, uint16_t wIdx, unsigned char *data, uint16_t wLen) {
	int		rc;

    if ((type & 0x80) == 0x80) {
        fprintf(stderr, "bmRequestType(0x%x) is not for output\n", type);
        return -1;
    }	
    rc = libusb_control_transfer(devh, type, req ,wVal, wIdx, data, wLen, UNLIMITED_TIMEOUT);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }
	return rc;
}


/*
 * rx_control(type, req, wVal, wIdx, data, wLen)
 *  function:   receive control data (send request, and receive data)
 *  args:       type:   bmRequestType
 *              req:    bRequestCode
 *              wVal:   wValue
 *              wIdx:   wIndex
 *              data:   data to to be transmitted
 *              wLen:   wLength
 *  return:     success(0) / fail (<0)
 */
int rx_control(uint8_t type, uint8_t req , uint16_t wVal, uint16_t wIdx, 
			   unsigned char *data, uint16_t wLen) {
	int		rc;

    if ((type & 0x80) == 0) {
        fprintf(stderr, "bmRequestType(0x%x) is not for input\n", type);
        return -1;
    }
	rc = libusb_control_transfer(devh, type, req ,wVal, wIdx, data, wLen, UNLIMITED_TIMEOUT);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
    }

	return rc;
}


/*
 * rx_interrupt(ep, data, size)
 *  function:   receive interrupt data
 *  args:       ep:     end-point to receive
 *              data:   pointer to store received data
 *              size:   receive buffer size
 *  return:     success(0) / fail (<0)
 */
 int rx_interrupt(int ep, unsigned char *data, int size)
{
	int	rc;
    int len;
    int i;

    rc = libusb_interrupt_transfer(devh, ep, data, size, &len, 2000);
    if (rc == LIBUSB_ERROR_TIMEOUT) {
        fprintf(stderr, "[Rx] libusb_interrupt_transfer() timeout.\n");
        return rc;
    } else if (rc < 0) {
        fprintf(stderr, "[Rx] libusb_interrupt_transfer() failed. (%d)\n", rc);
        return rc;
    }

    fprintf(stdout, "Received: ");
    for (i = 0; i < len; i++) {
        fprintf(stdout, "%02x ", *(data + i));
    }
    fprintf(stdout, "\n");

    return len;
}


/*
 * tx_bulk(ep, data, size)
 *  function:   transfer bulk data
 *  args:       ep:     end-point to receive
 *              data:   pointer to store received data
 *              size:   data size
 *  return:     transfered length(>=0) / fail (<0)
 */
 int tx_bulk(unsigned char ep, unsigned char *data, int len)
{
    int rc;
    int xfered;
    
    rc = libusb_bulk_transfer(devh, ep, data, len, &xfered, UNLIMITED_TIMEOUT);
    if (rc < 0) {
        fprintf(stderr, "[Tx] libusb_bulk_transfer() failed. (%d)\n", rc);
        return rc;
    }
    return xfered;
}


/*
 * rx_bulk(ep, data, size)
 *  function:   transfer bulk data
 *  args:       ep:     end-point to receive
 *              data:   pointer to store received data
 *              size:   received size
 *  return:     received length(>=0) / fail (<0)
 */
int rx_bulk(unsigned char ep, unsigned char *data, int len, int timeout)
{
    int rc;
    int xfered;
	int	i;
    
    rc = libusb_bulk_transfer(devh, ep, data, len, &xfered, timeout);
    if (rc == LIBUSB_ERROR_TIMEOUT) {
        fprintf(stderr, "[Rx] libusb_bulk_transfer() timeout\n");
        return rc;
    } else if (rc < 0) {
        fprintf(stderr, "[Rx] libusb_bulk_transfer() for rx failed. (%d)\n", rc);
        return rc;
    }

    fprintf(stdout, "Received: ");
    for (i = 0; i < xfered; i++) {
        fprintf(stdout, "%02x ", *(data+i));
    }
    fprintf(stdout, "\n");

    return xfered;
}

/*
 * Send_CDC_Data
 *  function:   send CDC OUT Data (Endpoint = 0x07)
 *  args:       
 *  return:     
 */
int Send_CDC_Data(unsigned char *data, int len)
{
	int rc;
    /* CDC OUT Data */
    rc = tx_bulk(0x07, data, len);
    if (rc < 0) {
        fprintf(stderr, "CDC OUT Data failed(%d).\n", rc);
        return rc;
    }

    return rc;
}

/*
 * Recv_CDC_Data
 *  function:   receive CDC IN Data (Endpoint = 0x85)
 *  args:       none
 *  return:     success / error
 */
int Recv_CDC_Data(void)
{
	int	rc;

	rc = rx_bulk(0x85, buf, 0xFF, 5000);
    if (rc < 0) {
        fprintf(stderr, "CDC IN Data failed(%d).\n", rc);
    }

	return rc;
}


/* main */
int main(int argc, char **argv)
{
    int rc;
    
    /* Initialize libusb */
	rc = init_usblib();
	CHECK_ERR(rc);

    /* get device handle
       note: this function waits until given device is active */
    devh = get_device_handle(VENDOR_ID, PRODUCT_ID);
    if (devh == NULL) {
        goto out;
    }

    // get Device Descriptor
    rx_control(0x80, 0x06 , 0x0100, 0x00, NULL, 0x40);

    
out:
    if (devh)
            libusb_close(devh);
    libusb_exit(NULL);
    return rc;
}
