Getting Started 
================

The following instructions explain how to build the demo for the
XR-AVB-LC-BRD endpoint.

To install the software, open the xTIMEcomposer Studio and
follow these steps:

#. Choose `File` |submenu| `Import`.

#. Choose `General` |submenu| `Existing Projects into Workspace` and
   click **Next**.

#. Click **Browse** next to `Select archive file` and select
   the file firmware ZIP file.

#. Make sure that all projects are ticked in the
   `Projects` list.
 
#. Click **Finish**.

To build, select the ``app_simple_avb_demo`` project in the
Project Explorer and click the **Build** icon.

.. cssclass:: cmd-only

From the command line, you can follow these steps:

#. To install, unzip the pacakge zipfile

#. To build, change into the ``app_simple_avb_demo`` directory and
   execute the command::

        xmake all

Makefiles
~~~~~~~~~

The main Makefile for the project is in the
``app_simple_avb_demo`` directory. This file specifies build
options and used modules.

Running the application
-----------------------

To upgrade the firmware you must, firstly connect the XTAG-2 to the 
relevant development board and plug the XTAG-2 into your PC or Mac.

Using the XMOS xTIMEcomposer Studio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using the 12.0.0 tools or later and AVB version 5.2.0 or
later, from within the xTIMEcomposer Studio:

 #. Right click on the binary within the bin folder of the project.
 #. Choose `Run As` |submenu| `Run Configurations`
 #. Double click `xCORE Application` in the left panel
 #. Choose `hardware` in `Device options` and select the relevant XTAG-2 adapter
 #. Select the `Run XScope output server` check box.
 #. Click on **Apply** if configuration has changed
 #. Click on **Run**

Using the Command Line Tools
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Open the XMOS command line tools (Command Prompt) and
   execute the following command:


   ::

       xrun --xscope <binary>.xe

#. If multiple XTAG2s are connected, obtain the adapter ID integer by executing:

   :: 

      xrun -l

#. Execute the `xrun` command with the adapter ID flag

   :: 

      xrun --id <id> --xscope <binary>.xe



Installing the application onto flash
-------------------------------------

#. Connect the XTAG-2 debug adapter to the relevant development
   board, then plug the XTAG-2 into your PC or Mac.


Using the XMOS xTIMEcomposer Studio
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To upgrade the flash from the xTIMEcomposer Studio, follow these steps:


#. Start the xTIMEcomposer Studio and open the workspace created in **Running the application**.
#. Right click on the binary within the bin folder of the project.
#. Choose `Flash As` |submenu| `Flash Configurations`
#. Double click `xCORE Application` in the left panel
#. Choose `hardware` in `Device options` and select the relevant XTAG-2 adapter
#. Click on **Apply** if configuration has changed
#. Click on **Flash**

Using Command Line Tools
~~~~~~~~~~~~~~~~~~~~~~~~


#. Open the XMOS command line tools (Command Prompt) and
   execute the following command:

   ::

       xflash <binary>.xe

#. If multiple XTAG2s are connected, obtain the adapter ID integer by executing:

   :: 

      xrun -l

#. Execute the `xflash` command with the adapter ID flag

   :: 

      xflash --id <id> <binary>.xe

