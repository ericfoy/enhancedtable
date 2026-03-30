(function ($, Backdrop) {
  'use strict';

  Backdrop.behaviors.enhancedtableLivePreview = {
    attach: function (context) {
      var $context = $(context);

      // ---- EnhancedTable cards: left/right "kick" ordering ----
      $context.find('.bt-cards').once('enhancedtable-cards').each(function () {
        var $cards = $(this);
        var btReorderBusy = false;

        function setReorderBusy(state) {
          btReorderBusy = state;

          // Temporarily suppress further move clicks while the reorder cycle finishes.
          $cards.find('.bt-move-left, .bt-move-right').prop('disabled', state);
        }

        function releaseReorderBusySoon(delay) {
          var old = $cards.data('btReorderBusyTimer');
          if (old) {
            window.clearTimeout(old);
          }

          $cards.data('btReorderBusyTimer', window.setTimeout(function () {
            setReorderBusy(false);
            refreshCardUIState();
          }, delay));
        }

        // Respect users who prefer reduced motion.
        var prefersReducedMotion = false;
        try {
          prefersReducedMotion = !!(window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches);
        }
        catch (e) {
          prefersReducedMotion = false;
        }

        function flashMoved($card, direction) {
          // Clear previous flash.
          $cards.children('.bt-card').removeClass('bt-moved bt-moved-left bt-moved-right');

          var dirClass = (direction === 'left') ? 'bt-moved-left' : 'bt-moved-right';
          $card.addClass('bt-moved ' + dirClass);

          // Remove after a short delay so the user can track it.
          var oldTimer = $card.data('btFlashTimer');
          if (oldTimer) {
            window.clearTimeout(oldTimer);
          }
          $card.data('btFlashTimer', window.setTimeout(function () {
            $card.removeClass('bt-moved bt-moved-left bt-moved-right');
          }, 3950));
        }

        // Animate reordering using a lightweight FLIP technique so the move is visible.
        function animateReorder(doMoveFn, $movedCard, direction) {
          if (btReorderBusy) {
            return;
          }

          setReorderBusy(true);

          if (prefersReducedMotion) {
            doMoveFn();
            refreshCardUIState();
            flashMoved($movedCard, direction);
            setReorderBusy(false);
            return;
          }

          var $before = $cards.children('.bt-card');
          var firstRects = [];
          $before.each(function () {
            firstRects.push({ el: this, rect: this.getBoundingClientRect() });
          });

          doMoveFn();
          refreshCardUIState();

          var moved = [];
          var $after = $cards.children('.bt-card');

          function findFirstRect(el) {
            for (var i = 0; i < firstRects.length; i++) {
              if (firstRects[i].el === el) {
                return firstRects[i].rect;
              }
            }
            return null;
          }

          $after.each(function () {
            var first = findFirstRect(this);
            if (!first) {
              return;
            }
            var last = this.getBoundingClientRect();
            var dx = first.left - last.left;
            var dy = first.top - last.top;

            if (dx || dy) {
              moved.push({ el: this });
              this.style.transition = 'none';
              this.style.transform = 'translate(' + dx + 'px, ' + dy + 'px)';
              this.style.willChange = 'transform';
            }
          });

          // Force reflow.
          $cards[0] && $cards[0].offsetHeight;

          (window.requestAnimationFrame || window.setTimeout)(function () {
            for (var i = 0; i < moved.length; i++) {
              var el = moved[i].el;
              el.style.transition = 'transform 360ms ease';
              el.style.transform = '';

              (function (node) {
                var old = $(node).data('btFlipTimer');
                if (old) {
                  window.clearTimeout(old);
                }
                $(node).data('btFlipTimer', window.setTimeout(function () {
                  node.style.transition = '';
                  node.style.transform = '';
                  node.style.willChange = '';
                }, 440));
              })(el);
            }

            flashMoved($movedCard, direction);

            // Release after the transition window has safely passed.
            releaseReorderBusySoon(460);
          }, 0);
        }
        // FLIP animation for reordering many cards at once (e.g. reset).
        function animateReorderAll(doMoveFn) {
          if (btReorderBusy) {
            return;
          }

          setReorderBusy(true);

          if (prefersReducedMotion) {
            doMoveFn();
            refreshCardUIState();
            setReorderBusy(false);
            return;
          }

          var $before = $cards.children('.bt-card');
          var firstRects = [];
          $before.each(function () {
            firstRects.push({ el: this, rect: this.getBoundingClientRect() });
          });

          doMoveFn();
          refreshCardUIState();

          var moved = [];
          var $after = $cards.children('.bt-card');

          function findFirstRect(el) {
            for (var i = 0; i < firstRects.length; i++) {
              if (firstRects[i].el === el) {
                return firstRects[i].rect;
              }
            }
            return null;
          }

          $after.each(function () {
            var first = findFirstRect(this);
            if (!first) {
              return;
            }
            var last = this.getBoundingClientRect();
            var dx = first.left - last.left;
            var dy = first.top - last.top;
            if (dx || dy) {
              moved.push({ el: this });
              this.style.transition = 'none';
              this.style.transform = 'translate(' + dx + 'px, ' + dy + 'px)';
              this.style.willChange = 'transform';
            }
          });

          $cards[0] && $cards[0].offsetHeight;

          (window.requestAnimationFrame || window.setTimeout)(function () {
            for (var i = 0; i < moved.length; i++) {
              var el = moved[i].el;
              el.style.transition = 'transform 420ms ease';
              el.style.transform = '';

              (function (node) {
                var old = $(node).data('btFlipTimer');
                if (old) {
                  window.clearTimeout(old);
                }
                $(node).data('btFlipTimer', window.setTimeout(function () {
                  node.style.transition = '';
                  node.style.transform = '';
                  node.style.willChange = '';
                }, 500));
              })(el);
            }

            releaseReorderBusySoon(520);
          }, 0);
        }

        function flashReset() {
          if (prefersReducedMotion) {
            return;
          }
          $cards.removeClass('bt-reset-flash');
          // Force reflow to restart animation.
          // eslint-disable-next-line no-unused-expressions
          $cards[0] && $cards[0].offsetHeight;
          $cards.addClass('bt-reset-flash');
          var oldTimer = $cards.data('btResetTimer');
          if (oldTimer) {
            window.clearTimeout(oldTimer);
          }
          $cards.data('btResetTimer', window.setTimeout(function () {
            $cards.removeClass('bt-reset-flash');
          }, 950));
        }

        function updateWeights() {
          $cards.children('.bt-card').each(function (i) {
            // Use 10-based weights for readability.
            var w = i * 10;
            $(this).find('input.bt-field-weight').val(w);
          });
        }

        function updateArrowState() {
          var $all = $cards.children('.bt-card');
          $all.find('.bt-move-left, .bt-move-right').prop('disabled', false);
          $all.first().find('.bt-move-left').prop('disabled', true);
          $all.last().find('.bt-move-right').prop('disabled', true);
        }

        function updateMergeLeftDisable() {
          var $all = $cards.children('.bt-card');

          // Enable all merge-left checkboxes first.
          $all.find('input.bt-merge-left').prop('disabled', false);

          // Disable merge-left on the left-most card (keep it visible).
          var $first = $all.first();
          var $firstMerge = $first.find('input.bt-merge-left');

          if ($firstMerge.length) {
            // If it was checked (e.g. after moving), force it off and refresh states.
            if ($firstMerge.is(':checked')) {
              $firstMerge.prop('checked', false).trigger('change');
            }
            $firstMerge.prop('disabled', true);
          }
        }

        // Stock Views table behavior: only one separator is used per combined
        // column. EnhancedTable collects it on the first merged child field.
        // Therefore:
        // - hide/disable separator inputs on non-merged fields
        // - show/enable separator input only on the first merged child in each
        //   merged group
        // - hide/disable separator inputs on subsequent merged children
        function updateSeparatorVisibility() {
          var $all = $cards.children('.bt-card');
          var seenChild = false;

          $all.each(function () {
            var $card = $(this);
            var merged = $card.find('input.bt-merge-left').is(':checked');
            var $sepInput = $card.find('input.bt-separator-input');
            if (!$sepInput.length) {
              // No separator control on this card.
              if (!merged) {
                seenChild = false;
              }
              return;
            }

            var $wrap = $sepInput.closest('.form-item, .form-wrapper, .form-type-textfield');
            if (!$wrap.length) {
              $wrap = $sepInput.parent();
            }
            if (!merged) {
              // New parent column.
              seenChild = false;
              $sepInput.prop('disabled', true);
              $wrap.addClass('bt-sep-hidden');
              return;
            }

            if (!seenChild) {
              // First merged child in this group.
              seenChild = true;
              $sepInput.prop('disabled', false);
              $wrap.removeClass('bt-sep-hidden').css('display', '');
            }
            else {
              // Subsequent merged child: no separator allowed.
              $sepInput.prop('disabled', true);
              $wrap.addClass('bt-sep-hidden');
            }
          });
        }

        function refreshCardUIState() {
          updateWeights();
          updateArrowState();
          updateMergeLeftDisable();
          updateSeparatorVisibility();
        }

        // Ensure weights match DOM order on attach.
        refreshCardUIState();

        // In the Views modal, Backdrop/states.js may toggle form elements after
        // behaviors attach. Re-run once on the next tick so separator visibility
        // is correct on initial open.
        (window.requestAnimationFrame || window.setTimeout)(function () {
          refreshCardUIState();
        }, 0);

        // Recompute separator visibility when merge-left is toggled.
        $cards.on('change', 'input.bt-merge-left', function () {
          refreshCardUIState();
        });

        $cards
          .off('click.enhancedtableMoveLeft', '.bt-move-left')
          .on('click.enhancedtableMoveLeft', '.bt-move-left', function (e) {
            e.preventDefault();
            e.stopPropagation();
            e.stopImmediatePropagation();

            if (btReorderBusy) {
              return false;
            }

            var $card = $(this).closest('.bt-card');
            var $prev = $card.prev('.bt-card');

            if ($prev.length) {
              console.log('move-left fired for', $card.attr('data-bt-field'));
              animateReorder(function () {
                $card.insertBefore($prev);
              }, $card, 'left');
            }

            return false;
          });

        $cards
          .off('click.enhancedtableMoveRight', '.bt-move-right')
          .on('click.enhancedtableMoveRight', '.bt-move-right', function (e) {
            e.preventDefault();
            e.stopPropagation();
            e.stopImmediatePropagation();

            if (btReorderBusy) {
              return false;
            }

            var $card = $(this).closest('.bt-card');
            var $next = $card.next('.bt-card');

            if ($next.length) {
              console.log('move-right fired for', $card.attr('data-bt-field'));
              animateReorder(function () {
                $card.insertAfter($next);
              }, $card, 'right');
            }

            return false;
          });
        // Reset to query order (Views "Fields | Rearrange").
        var $resetBtn = $cards.closest('#backdrop-modal').find('button.bt-reset-field-order').once('bt-reset-field-order');
        $resetBtn.on('click', function (e) {
          e.preventDefault();

          var items = $cards.children('.bt-card').get();
          items.sort(function (a, b) {
            var ia = parseInt(a.getAttribute('data-bt-query-index') || '0', 10);
            var ib = parseInt(b.getAttribute('data-bt-query-index') || '0', 10);
            if (ia === ib) {
              var fa = a.getAttribute('data-bt-field') || '';
              var fb = b.getAttribute('data-bt-field') || '';
              return fa.localeCompare(fb);
            }
            return ia - ib;
          });

          animateReorderAll(function () {
            for (var i = 0; i < items.length; i++) {
              $cards.append(items[i]);
            }
          });

          refreshCardUIState();
          flashReset();
        });
      });

      // ---- Flipped matrix live preview (existing logic) ----
      var $matrix = $context
        .find('table.bt-matrix')
        .once('enhancedtable-live-preview');

      if ($matrix.length) {
        // Scope for finding the checkboxes.
        var $scope = $matrix.closest('#backdrop-modal');
        if (!$scope.length) {
          $scope = $context;
        }

        function syncMatrixClasses() {
          var rowBorders = $scope
            .find('input[name="style_options[row_borders]"]')
            .is(':checked');

          var colBorders = $scope
            .find('input[name="style_options[column_borders]"]')
            .is(':checked');

          var frameBorder = $scope
            .find('input[name="style_options[frame_border]"]')
            .is(':checked');

          var striping = $scope
            .find('input[name="style_options[row_class_special]"]')
            .is(':checked');

          $matrix
            .toggleClass('bt-row-borders', rowBorders)
            .toggleClass('bt-column-borders', colBorders)
            .toggleClass('bt-frame-border', frameBorder)
            .toggleClass('bt-striping', striping);
        }

        // Initial state when dialog opens / AJAX updates.
        syncMatrixClasses();

        // Update on changes to the relevant checkboxes.
        $scope
          .find(
            'input[name="style_options[row_borders]"], ' +
            'input[name="style_options[column_borders]"], ' +
            'input[name="style_options[frame_border]"], ' +
            'input[name="style_options[row_class_special]"]'
          )
          .once('enhancedtable-live-preview-events')
          .on('change', syncMatrixClasses);
      }

      // ---- Upgrade color textfields to native color pickers ----
      $(context)
        .find('input.bt-color-input')
        .once('bt-color-input')
        .each(function () {
          try {
            this.type = 'color';
          }
          catch (e) {
            // leave as text
          }
        });
    }
  };

})(jQuery, Backdrop);