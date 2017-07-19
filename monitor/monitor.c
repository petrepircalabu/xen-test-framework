#include <stdio.h>
#include <stdlib.h>

#include <xenctrl.h>
#include <xenevtchn.h>

int main(int argc, char* argv[])
{
    domid_t domain_id;
    xc_interface *xch;

    domain_id = atoi(argv[0]);

    xch = xc_interface_open(NULL, NULL, 0);
    if ( !xch )
    {
        printf("Error initialising xenaccess\n");
        return 1;
    }

    return 0;
}
