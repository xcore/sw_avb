Getting Started 
================

The following instructions show how to build the demo for the
XR-AVB-LC-BRD endpoint. The same method holds for the other
applications.

.. only:: xdehtml

   .. raw:: html
 
     <div class="xde-inside">
     <ul class="iconmenu">
       <li class="xde"><a href="http://www.xmos.com/automate?automate=ImportComponent&partnum=XR-000008-SW">Import AVB Reference Design</a></li>
     </ul>
     </div>

  To build, select the ``app_usb_aud_l1`` or ``app_usb_aud_l2`` project in the
  Project Explorer and click the **Build** icon.

.. cssclass:: xde-outside

  To install the software, open the XDE (XMOS Development Tools) and
  follow these steps:

  #. Choose `File` |submenu| `Import`.

  #. Choose `General` |submenu| `Existing Projects into Workspace` and
     click **Next**.

  #. Click **Browse** next to `Select archive file` and select
     the file firmware ZIP file.

  #. Make sure the projects you want to import are ticked in the
     `Projects` list. Import all the components and whichever
     applications you are interested in.
   
  #. Click **Finish**.

  To build, select the ``app_xr_avb_lc_demo`` project in the
  Project Explorer and click the **Build** icon.

.. cssclass:: cmd-only

  From the command line, you can follow these steps:

  #. To install, unzip the pacakge zipfile

  #. To build, change into the ``app_xr_avb_lc_demo`` directory and
     execute the command::

          xmake all

Makefiles
~~~~~~~~~

The main Makefile for the project is in the
``app_xr_avb_lc_demo`` directory. This file specifies build
options and used modules. The Makefile uses the common build
infrastructure in ``module_xmos_common``. This system includes
the source files from the relevant modules and is documented within
``module_xmos_common``.

Running the application
-----------------------

To upgrade the firmware you must, firstly connect the XTAG-2 to the 
relevant development board and plug the XTAG-2 into your PC or Mac.

Using the XMOS Development Environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using the 11.2.0 development tools or later and AVB version 5v0.1 or
later, from within the XDE:

 #. Right click on the binary within the project.
 #. Choose `Run As` |submenu| `Run Configurations`
 #. Choose `hardware` and select the relevant XTAG-2 adapter
 #. Select the `Run UART server` check box.
 #. Click on **Apply** if configuration has changed
 #. Click on **Run**

Using the Command Line Tools
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Open the XMOS command line tools (Desktop Tools Prompt) and
   execute the following command:


   ::

       xrun --uart <binary>.xe


Installing the application onto flash
-------------------------------------

#. Connect the XTAG-2 debug adapter to the  relevant development
   board, then plug the XTAG-2 into your PC or Mac.


Using the XMOS Development Environment
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To upgrade the flash from the XDE, follow these steps:


#. Start the XMOS Development Environment and open a workspace.

#. Choose *File* |submenu| *Import* |submenu| *C/XC* |submenu| *C/XC Executable*

#. Click **Browse** and select the new firmware (XE) file.

#. Click **Next** and **Finish**.

#. A Debug Configurations window is displayed. Click **Close**.

#. Choose *Run* |submenu| *Run Configurations*.

#. Double-click *Flash Programmer* to create a new
   configuration.

#. Browse for the XE file in the *Project* and
   *C/XC Application* boxes.

#. Ensure the *XTAG-2* device appears in the adapter
   list.

#. Click **Run**.


Using Command Line Tools
~~~~~~~~~~~~~~~~~~~~~~~~


#. Open the XMOS command line tools (Desktop Tools Prompt) and
   execute the following command:

   ::

       xflash <binary>.xe


