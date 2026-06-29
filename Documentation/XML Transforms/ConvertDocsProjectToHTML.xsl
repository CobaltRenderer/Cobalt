<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:doc="http://www.cobaltrenderer.com/schema/XMLDocSchema.xsd">

  <!-- Specify the output document format settings -->
  <xsl:output method="html" indent="yes" encoding="UTF-8" doctype-system="about:legacy-compat"/>
  <xsl:strip-space elements="*"/>

  <!-- Process the root "XMLDocProject" element -->
  <xsl:template match="/doc:XMLDocProject">
    <html>
      <head>
        <title>
          <xsl:value-of select="@Title"/>
        </title>
        <link href="images/favicon.ico" rel="icon"/>
        <link href="styles/DocStyle.css" rel="stylesheet" type="text/css"/>
        <script><![CDATA[
    window.addEventListener('message', (event) => { window.history.replaceState(null, null, window.location.pathname + '?page=' + event.data); }, false);
    window.onload = function loadedFunction() { 
    if (window.top.location.search != '') {
      document.getElementsByName("docFrame")[0].src = 'html/' + new URLSearchParams(window.top.location.search).get('page') + '.html';
    }
  }]]>
        </script>
      </head>
      <body class="mainBody">
        <div class="mainFrameSet">
          <iframe src="html/TableOfContents.html" name="tocFrame" class="tocFrameClass"></iframe>
          <iframe src="html/{@InitialPage}.html" name="docFrame" class="docFrameClass"></iframe>
        </div>
      </body>
    </html>
  </xsl:template>

</xsl:stylesheet>
