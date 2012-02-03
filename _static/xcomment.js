function show_comments(slot_id) {
   $('*[id|=xcomment-expand-'+slot_id+']').hide();
   $('*[id|=xcomment-'+slot_id+']').show();
   $('*[id|=xcomment-post-'+slot_id+']').show();
   $('*[id|=xcomment-form-'+slot_id+']').hide();
}

function hide_comments(slot_id) {
   $('*[id|=xcomment-expand-'+slot_id+']').show();
   $('*[id|=xcomment-'+slot_id+']').hide();
   $('*[id|=xcomment-post-'+slot_id+']').hide();
   $('*[id|=xcomment-form-'+slot_id+']').hide();
}

function hide_all_comments() {
   $('*[id|=xcomment]').hide();
}

function show_all_comments() {
   $('*[id|=xcomment]').hide();
   $('*[id|=xcomment-expand]').show();
   $('*[id|=xcomment-inline-add]').show();
   $('*[id|=xcomment-post]').hide(); 
}

function expand_all_comments() {
   $('*[id|=xcomment]').show();
   $('*[id|=xcomment-expand]').hide();
   $('*[id|=xcomment-form]').hide();
   $('*[id|=xcomment-post]').show(); 
   $('*[id|=xcomment-post-form-only]').hide(); 
}

function show_comment_form(slot_id) {
    $('#xcomment-form-'+slot_id).show()
    $('*[id|=xcomment-post-'+slot_id+']').show();
}


$(function() {
    $('*[id|=xcomment-submit]').each(function() {
         $(this).click(function() {
                 // validate and process form here
                 var this_id = $(this).attr("id");
                 var slot_id = this_id.slice(this_id.lastIndexOf("-")+1);
                 var name = $("input#xcomment-input-name-"+slot_id).val();
                 var email = $("input#xcomment-input-email-"+slot_id).val();
                 var comments = $("input#xcomment-input-email-"+slot_id).val();
                 var line = $("input#xcomment-input-line-"+slot_id).val();
                 var page = $("input#xcomment-input-page-"+slot_id).val();
                 var doc = $("input#xcomment-input-doc-"+slot_id).val();
                 
                 var dataString = $("#xcomment-form-form-"+slot_id).serialize();
                  $.ajax({
                          type: "POST",
                          url: "../../xcomment_web.py/submit_comment_ajax",
                          data: dataString,
                          success: function(data) {
                              obj = jQuery.parseJSON(data);
                              $('#xcomment-expand-'+slot_id).replaceWith(obj.new_expand);
                              $('#xcomment-inline-add-'+slot_id).replaceWith(obj.new_inline);
                              $('#xcomment-post-form-only-'+slot_id).replaceWith(obj.new_post);
                              $('#xcomment-insert-point-'+slot_id).before(obj.new_comment);
                              $('#xcomment-form-'+slot_id).hide();
                              show_comments(slot_id);
                             
                                  
                                   }
                        });
                 return false;
             })
              })
});
